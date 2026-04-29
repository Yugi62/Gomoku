#include "Server.h"
#include "Utility.h"
#include "Database.h"

Session::Session(
	boost::asio::ip::tcp::socket socket, 
	boost::asio::ssl::context& context, 
	boost::asio::strand<boost::asio::io_context::executor_type>& strand,
	IServer* iServer,
	int sessionId
	) :	
	_socket(std::move(socket), context),
	_strand(strand),
	_iServer(iServer),
	_sessionId(sessionId)
{
	_forDispatch["Login"] = [this](nlohmann::json& j) {Start_Login(j); };
	_forDispatch["Register"] = [this](nlohmann::json& j) {Start_Register(j); };
	_forDispatch["Check_Username"] = [this](nlohmann::json& j) {Check_UserName(j); };
	_forDispatch["Check_UserNickname"] = [this](nlohmann::json& j) {Check_UserNickName(j); };
	_forDispatch["Chat"] = [this](nlohmann::json& j) {Start_Chat(j); };
	_forDispatch["Create_Room"] = [this](nlohmann::json& j) {Create_Room(j); };
	_forDispatch["Exit_Room"] = [this](nlohmann::json& j) {Exit_Room(j); };
	_forDispatch["Refresh_Room"] = [this](nlohmann::json& j) {Refresh_Room(j); };
	_forDispatch["Join_Room"] = [this](nlohmann::json& j) {Join_Room(j); };
	_forDispatch["Room_Chat"] = [this](nlohmann::json& j) {Room_Chat(j); };
	_forDispatch["Room_Ready"] = [this](nlohmann::json& j) {Room_Ready(); };
}

void Session::Start()
{
	Start_Handshake();
}

void Session::Start_Handshake()
{
	_socket.async_handshake(boost::asio::ssl::stream_base::server,
		[this](const boost::system::error_code& error)
		{
			//핸드쉐이크 성공 시
			if (!error)
			{
				Start_Read();
			}
			else
				//세션 파괴 (실패 시 통신 종료)
				_iServer->Destroy_Session(_sessionId);
		}
	);
}

void Session::Start_Read()
{
	//글자 수 읽기 (4바이트)
	_socket.async_read_some(boost::asio::buffer(_buf),
		[this](const boost::system::error_code& error, std::size_t len1)
		{
			if (!error)
			{
				//쓰레기 값이 들어갈 수 있으므로 string으로 변환 시 딱 len만큼만 넣는다
				std::string str1(_buf.data(), len1);

				//크기를 정수로 파싱
				int size = std::stoi(str1);

				//실제 데이터가 저장되는 버퍼
				auto dataBuf = std::make_shared<std::vector<char>>(size);

				//글자 수를 토대로 데이터 읽기
				_socket.async_read_some(boost::asio::buffer(*dataBuf),
					[this, dataBuf](const boost::system::error_code& error, std::size_t len2)
					{
						if (!error)
						{
							//읽기 재시작
							Start_Read();

							//쓰레기 값이 들어갈 수 있으므로 string으로 변환 시 딱 len만큼만 넣는다
							std::string str2(dataBuf->data(), len2);

							//받은 string을 처리
							Start_Dispatch(str2);
						}
					});
			}
			else			
				//세션 파괴 (Read를 더 이상 할 수 없으므로 통신 종료)
				_iServer->Destroy_Session(_sessionId);
		}
	);
}

void Session::Start_Write(std::string str)
{
	//비동기 작업이 완료될 때까지 버퍼가 살아있어야하므로 스마트 포인터로 초기화
	auto buffer = std::make_shared<std::string>(str);

	//strand로 묶어서 중복 실행 방지
	_socket.async_write_some(boost::asio::buffer(*buffer),
		boost::asio::bind_executor(_strand,
			[this, buffer](const boost::system::error_code& error, std::size_t len)
			{
				if (!error)
				{
					/*

					Write 완료

					*/
				}
			}
		));
}

void Session::Send(std::string str)
{
	Start_Write(str);
}

Server::Server(boost::asio::io_context& io, unsigned short port) :
	_acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	_context(boost::asio::ssl::context::sslv23),
	_strand(boost::asio::make_strand(io))
{
	_context.set_options(
		boost::asio::ssl::context::default_workarounds |				//우회 작업 허용
		boost::asio::ssl::context::single_dh_use |						//DH 파라미터 강제 새로고침
		boost::asio::ssl::context::no_sslv2 |							//sslv2 금지
		boost::asio::ssl::context::no_sslv3 |							//sslv3	금지
		boost::asio::ssl::context::no_tlsv1 |							//tlsv1.0 금지
		boost::asio::ssl::context::no_tlsv1_1							//tlsv1.1 금지
	);

	//핸드쉐이크를 위해 인증서와 키를 로드
	_context.use_certificate_file("Server.crt", boost::asio::ssl::context::pem);
	_context.use_private_key_file("Server.key", boost::asio::ssl::context::pem);

	//dh 키교환 방식으로 변경
	_context.use_tmp_dh_file("dh2048.pem");

	//비동기로 Accept 시작
	Start_Accept();
}

void Server::Start_Accept()
{
	_acceptor.async_accept(
		[this](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket)
		{
			//오류가 없으면 스마트 포인터로 세션을 생성
			if (!error)
			{
				int sessionId = Get_SessionId();
				_sessionMap[sessionId] = std::make_shared<Session>(std::move(socket), _context, _strand, this, sessionId);
				_sessionMap[sessionId]->Start();
			}

			//Accept 완료 후 다시 Accept 
			Start_Accept();
		}
	);
}

int Server::Get_SessionId()
{
	//큐에 반환된 Id가 존재할 경우 이 Id를 우선적으로 반환
	if (!_sessionQueue.empty())
	{
		int returnSessionId = _sessionQueue.front();
		_sessionQueue.pop();
		return returnSessionId;
	}
	//큐가 비어있는 경우 줄 수 있는 ID를 반환
	else
		return _nextSessionId++;
}

void Session::Start_Login(nlohmann::json& j)
{
	nlohmann::json newJ;

	newJ["type"] = "Login";

	//비밀번호 대조
	if (Utility::verifyPassword(j["password"], Database::GetInstance().GetPassword(j["id"])))
	{
		newJ["result"] = "True";
		newJ["nickname"] = Database::GetInstance().GetNickname(j["id"]);
		_nickname = newJ["nickname"];
	}
	else
	{
		newJ["result"] = "False";
	}

	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);
}

void Session::Start_Register(nlohmann::json& j)
{
	nlohmann::json newJ;
	newJ["type"] = "Register";

	//id와 nickname 유효성 검사 (둘 중 하나라도 DB에 존재하면 실패)
	if (Database::GetInstance().CheckUserid(j["id"]) || Database::GetInstance().CheckUserNickname(j["nickname"]))
	{
		newJ["result"] = "False";
	}
	else
	{
		Database::GetInstance().InsertUser(j["id"], j["password"], j["nickname"]);
		newJ["result"] = "True";
	}

	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);
}

void Session::Check_UserName(nlohmann::json& j)
{
	nlohmann::json newJ;
	newJ["type"] = "Check_Username";

	if (Database::GetInstance().CheckUserid(j["id"]))
	{
		newJ["result"] = "True";
	}
	else
		newJ["result"] = "False";

	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);
}

void Session::Check_UserNickName(nlohmann::json& j)
{
	nlohmann::json newJ;
	newJ["type"] = "Check_UserNickname";

	if (Database::GetInstance().CheckUserNickname(j["nickname"]))
	{
		newJ["result"] = "True";
	}
	else
		newJ["result"] = "False";

	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);
}

void Session::Start_Chat(nlohmann::json& j)
{
	nlohmann::json newJ;
	newJ["type"] = "Chat";
	newJ["chat"] = j["chat"];
	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;

	_iServer->Broadcast_Chat(newStr, _sessionId);
}

void Session::Create_Room(nlohmann::json& j)
{
	_roomId = _iServer->Create_Room(_sessionId, j["nickname"], j["name"], j["password"]);

	nlohmann::json newJ;
	newJ["type"] = "Create_Room";
	newJ["result"] = "True";
	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);

	Refresh_PlayerInfo(_roomId);
}

void Session::Exit_Room(nlohmann::json& j)
{
	_iServer->Get_Room(_roomId)->DeleteSessionId(_sessionId);
}

void Session::Refresh_Room(nlohmann::json& j)
{
	nlohmann::json newJ;
	newJ["type"] = "Refresh_Room";
	newJ["rooms"] = _iServer->Get_RoomJson();
	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);
}

void Session::Join_Room(nlohmann::json& j)
{
	nlohmann::json newJ;
	newJ["type"] = "Join_Room";

	int roomId = j["roomId"];
	std::string pass = j["password"];

	std::shared_ptr<Room> room = _iServer->Get_Room(roomId);

	//방 존재 유무 확인 + 비밀번호 대조
	if (room != nullptr && room->VerifyPassword(pass))
	{
		//세션에 id 입력
		_roomId = roomId;

		//방에 세션 id 추가
		room->AddSessionId(_sessionId, j["nickname"]);
		newJ["result"] = "True";
	}
	else
		newJ["result"] = "False";

	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);

	Refresh_PlayerInfo(_roomId);
}

void Session::Refresh_PlayerInfo(int roomId)
{	
	/*
	일단 대충 방에 소속된 플레이어의 닉네임을 배열로 넣어서 
	클라이언트한테 전송하는 느낌임	
	*/

	nlohmann::json newJ;

	newJ["type"] = "Refresh_PlayerInfo";

	newJ["nicknames"] = nlohmann::json::array();

	auto list = _iServer->Get_Room(roomId)->GetSessionIdList();
	for (auto it = list->begin(); it != list->end(); it++)
	{
		newJ["nicknames"].push_back(it->second);
	}

	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	Start_Write(newStr);
}

void Session::Room_Ready()
{
	nlohmann::json newJ;
	newJ["type"] = "Room_Chat";

	std::string str = "Server : ";
	str += _nickname + " is ready";

	newJ["chat"] = str;


	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;

	_iServer->Broadcast_Room_Chat(newStr, -1, _roomId);

	/*
	04.28

	일단 시간 없어서 Broadcast_Room_Chat를 썼는데

	나중에 따로 함수 만드셈

	특정 작업할 때마다 서버가 방 안에 모든 플레이어한테 메세지를 전송하는데
	Broadcast_Room_Chat 이거는 특정 id를 제외하고 보내는 방식이라 좀 다르게 작동함 

	그리고 준비는하는데 준비 푸는것도 만드셈
	
	
	
	*/

}

void Session::Room_Chat(nlohmann::json& j)
{
	nlohmann::json newJ;
	newJ["type"] = "Room_Chat";
	newJ["chat"] = j["chat"];

	std::string newStr = newJ.dump();
	newStr = Utility::fillZero(std::to_string(newStr.size()), 4) + newStr;
	
	_iServer->Broadcast_Room_Chat(newStr, _sessionId, _roomId);
}


struct T
{
	std::string str;
	int a;
};

void to_json(nlohmann::json& j, const T& t) 
{
	j = nlohmann::json{ {"str", t.str}, {"a", t.a} };
}

void Session::Start_Dispatch(std::string str)
{
	nlohmann::json j;
	j = nlohmann::json::parse(str);

	std::cout << str << '\n';

	//j["type"]에 따라 map에 연결된 function을 호출
	_forDispatch[j["type"]](j);
}

void Server::Destroy_Session(int sessionId)
{
	//Map에 세션이 존재하는 경우 삭제
	auto it = _sessionMap.find(sessionId);
	if (it != _sessionMap.end())
		_sessionMap.erase(it);

	//반환한 ID를 재사용할 수 있게 큐에 삽입
	_sessionQueue.push(sessionId);
}

void Server::Broadcast_Chat(std::string str, int id)
{
	for (auto it : _sessionMap)
	{
		if (it.first == id)
			continue;

		it.second->Send(str);
	}
}

int Server::Create_Room(int sessionId, std::string nickname, std::string roomName, std::string roomPassword)
{
	int returnRoomId;

	//큐에 반환된 Id가 존재할 경우 이 Id를 우선적으로 반환
	if (!_roomQueue.empty())
	{
		returnRoomId = _roomQueue.front();
		_roomQueue.pop();
	}
	//큐가 비어있는 경우 줄 수 있는 ID를 반환
	else
		returnRoomId = _nextRoomId++;

	//부여받은 Id로 Room 클래스 생성
	auto newRoom = std::make_shared<Room>(returnRoomId, roomName, roomPassword, [this](int roomId) { Destroy_Room(roomId); });

	//Id와 Room을 Map으로 연결
	_roomMap[returnRoomId] = newRoom;

	//Room에 sessionId 목록에 추가
	newRoom->AddSessionId(sessionId, nickname);

	return returnRoomId;
}

void Server::Destroy_Room(int roomId)
{
	//Map에 방이 존재하는 경우 삭제
	auto it = _roomMap.find(roomId);

	if (it != _roomMap.end())	
		_roomMap.erase(it);	

	//반환한 ID를 재사용할 수 있게 큐에 삽입
	_roomQueue.push(roomId);
}

std::shared_ptr<Room> Server::Get_Room(int roomId)
{
	if (_roomMap.find(roomId) != _roomMap.end())
		return _roomMap[roomId];
	else
		return nullptr;
}

std::vector<nlohmann::json> Server::Get_RoomJson()
{
	std::vector<nlohmann::json> v;

	for (auto it : _roomMap)	
		v.push_back(it.second->ReturnJson());	

	return v;
}

void Server::Broadcast_Room_Chat(std::string str, int id, int roomId)
{
	auto list = Get_Room(roomId)->GetSessionIdList();

	for (auto it = list->begin(); it != list->end(); it++)
	{
		int current = it->first;

		if (id != current)
		{
			_sessionMap[current]->Send(str);
		}
	}
}