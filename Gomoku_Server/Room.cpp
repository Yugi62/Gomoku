#include <algorithm>
#include <random>

#include "Room.h"
#include "Utility.h"

const int board_size = 15;

Room::Room(IServer* iServer, int roomId, std::string roomName, std::string roomPassword, std::function<void(int)> func, RedisManager& redisManager)
	: _iServer(iServer), _roomId(roomId), _roomName(roomName), gomoku(board_size), _redisManager(redisManager)
{
	if (!roomPassword.empty())
	{
		_isPrivate = true;
		_roomPassword = roomPassword;
	}
	else
		_isPrivate = false;

	_destroy_room_callback = func;
}

void Room::AddSessionId(int sessionId, std::string nickname)
{
	_sessionIdList.push_back(Player{ sessionId, nickname, false });
}

void Room::DeleteSessionId(int sessionId)
{
	//삭제 후 vector가 빈 경우 room을 삭제
	//map에서도 삭제해야함
	//id도 반환해야하고

	auto it = find_if(_sessionIdList.begin(), _sessionIdList.end(),
		[&sessionId](const Player& p) { return p.sessionId == sessionId; });

	if (it != _sessionIdList.end())
		_sessionIdList.erase(it);

	//세션이 빈 경우 서버에게 방 삭제를 요청 (map에서 삭제되면 참조 카운터가 0이 되면서 삭제)
	if (_sessionIdList.empty())
	{
		std::cout << "[ID : " << _roomId << "] " << "RoomName = " << _roomName << " has been deleted." << std::endl;

		_destroy_room_callback(_roomId);
	}
}

bool Room::VerifyPassword(std::string password)
{
	//방 비밀번호가 존재하지 않으면 바로 true 반환
	if (_roomPassword == "")
		return true;

	if (_roomPassword == password)
		return true;
	else
		return false;
}

const std::vector<Player>* Room::GetSessionIdList()
{
	return &_sessionIdList;
}

nlohmann::json Room::ReturnJson()
{
	nlohmann::json j;

	j["roomId"] = _roomId;
	j["roomName"] = _roomName;	
	j["isPrivate"] = _isPrivate ? "True" : "False";
	j["playerNumber"] = _sessionIdList.size();

	return j;
}

void Room::SetReady(int sessionId)
{
	auto it = find_if(_sessionIdList.begin(), _sessionIdList.end(),
		[&sessionId](const Player& p) { return p.sessionId == sessionId; });

	//현 준비상태를 반전
	it->isReady = !it->isReady;

	//준비상태인 경우 카운트를 올리고 아닌 경우 내림
	if (it->isReady)
		readyCnt++;
	else
		readyCnt--;

	//인원수만큼 카운트가 오른 경우 게임 시작
	int sz = _sessionIdList.size();
	if (sz >= 2 && sz == readyCnt)
	{
		std::cout << "Ready to play" << std::endl;

		//value에 랜덤으로 0 혹은 1을 초기화
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dist(0, 1);
		int value = dist(gen);

		//선택된 플레이어는 선공 그 반대는 후공으로 처리 (흑돌 = 1 / 백돌 = 2)
		_sessionIdList[value].isMyTurn = true;
		_sessionIdList[value].color = 1;	

		//적용된 내용을 클라이언트에게 전송
		for (int i = 0; i < _sessionIdList.size(); i++)
		{
			nlohmann::json j;
			j["type"] = "Game_Start";
			j["isMyTurn"] = _sessionIdList[i].isMyTurn ? "True" : "False";

			std::string str;
			str = j.dump();
			str = Utility::fillZero(std::to_string(str.size()), 4) + str;
			_iServer->Send_Data(str, _sessionIdList[i].sessionId);
		}
	}
}

void Room::Place_Gomoku_Stone(unsigned int sessionId, int x, int y)
{
	Player* player;
	Player* opponent;
	if (_sessionIdList[0].sessionId == sessionId)
	{
		player = &_sessionIdList[0];
		opponent = &_sessionIdList[1];
	}
	else
	{
		player = &_sessionIdList[1];
		opponent = &_sessionIdList[0];
	}

	//보드에 입력 및 검사
	std::string result = gomoku.Place_Stone(x, y, player->color);

	nlohmann::json __j;
	__j["type"] = "Room_Chat";

	//착수로 게임이 끝난 경우
	if (result != "")
	{
		Reset_Player_Status();

		std::string str;

		//오목으로 이긴 경우
		if (result == "Gomoku")
		{
			__j["chat"] = "Server : Gomoku";
			str = __j.dump();
			str = Utility::fillZero(std::to_string(str.size()), 4) + str;

			_redisManager.Update_Player(player->nickname, "Win", 1);
			_redisManager.Update_Player(opponent->nickname, "Loss", 1);
			_redisManager.Update_Player(player->nickname, "Rating", 10);
			_redisManager.Update_Player(opponent->nickname, "Rating", -5);
			_redisManager.Update_Ranking(player->nickname);
			_redisManager.Update_Ranking(opponent->nickname);
		}
		//금수로 진 경우
		else
		{
			if (result == "Overline")
			{
				__j["chat"] = "Server : Overline";
				str = __j.dump();
				str = Utility::fillZero(std::to_string(str.size()), 4) + str;
			}
			else if (result == "Three")
			{
				std::cout << "Three" << std::endl;
				__j["chat"] = "Server : Three";
				str = __j.dump();
				str = Utility::fillZero(std::to_string(str.size()), 4) + str;
			}
			else if (result == "Four")
			{
				std::cout << "Four" << std::endl;
				__j["chat"] = "Server : Four";
				str = __j.dump();
				str = Utility::fillZero(std::to_string(str.size()), 4) + str;
			}

			_redisManager.Update_Player(player->nickname, "Loss", 1);
			_redisManager.Update_Player(opponent->nickname, "Win", 1);
			_redisManager.Update_Player(player->nickname, "Rating", -5);
			_redisManager.Update_Player(opponent->nickname, "Rating", 10);
			_redisManager.Update_Ranking(player->nickname);
			_redisManager.Update_Ranking(opponent->nickname);
		}

		for (auto sess : _sessionIdList)
		{
			_iServer->Send_Data(str, sess.sessionId);
		}

		std::string st;
		nlohmann::json _j;
		_j["type"] = "Reset_Board";
		st = _j.dump();
		st = Utility::fillZero(std::to_string(str.size()), 4) + st;

		for (auto sess : _sessionIdList)
		{
			_iServer->Send_Data(st, sess.sessionId);
		}

	}
	//착수로 게임이 끝나지 않은 경우
	else
	{
		nlohmann::json j;
		j["type"] = "Place_Stone";
		j["x"] = x;
		j["y"] = y;
		j["color"] = player->color;

		std::string str;
		str = j.dump();
		str = Utility::fillZero(std::to_string(str.size()), 4) + str;
		_iServer->Send_Data(str, opponent->sessionId);

		std::cout << str << std::endl;
	}
}

void Room::Reset_Player_Status()
{
	std::cout << "Reset_Player_Status" << std::endl;

	readyCnt = 0;

	for (auto& it : _sessionIdList)
	{
		it.isMyTurn = false;
		it.isReady = false;
		it.color = 2;
	}
}