#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <queue>
#include <memory>
#include <array>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "Room.h"

class IServer;

class Session
{
private:
	//서버 인터페이스
	IServer* _iServer;
	//서버에서 각 세션을 구분하기 위한 ID
	unsigned int _sessionId;
	//이거 id만으로는 정확하게 room에 있는지 없는지 확인이 어려우니깐 bool이라도 만들어야함
	unsigned int _roomId;
	//닉네임
	std::string _nickname;

	std::unordered_map<std::string, std::function<void(nlohmann::json&)>> _forDispatch;

	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> _socket;
	boost::asio::strand<boost::asio::io_context::executor_type>& _strand;
	std::array<char, 4> _buf;

	void Start_Handshake();
	void Start_Read();
	void Start_Write(std::string str);

	//로그인 요청
	void Start_Login(nlohmann::json& j);
	//회원가입 요청
	void Start_Register(nlohmann::json& j);
	//회원가입 시 중복 ID 유무 검사
	void Check_UserName(nlohmann::json& j);
	//회원가입 시 중복 닉네임 유무 검사
	void Check_UserNickName(nlohmann::json& j);
	//채팅
	void Start_Chat(nlohmann::json& j);
	//방 생성
	void Create_Room(nlohmann::json& j);
	//방 나가기
	void Exit_Room(nlohmann::json& j);
	//방 최신화
	void Refresh_Room(nlohmann::json& j);
	//방 참가
	void Join_Room(nlohmann::json& j);
	//방 채팅
	void Room_Chat(nlohmann::json& j);
	//플레이어 정보 최신화
	void Refresh_PlayerInfo(int roomId);
	//플레이어 게임 준비
	void Room_Ready();
	//작업 선별
	void Start_Dispatch(std::string str);



public:
	Session(
		boost::asio::ip::tcp::socket socket, 
		boost::asio::ssl::context& context, 
		boost::asio::strand<boost::asio::io_context::executor_type>& strand,
		IServer* iServer,
		int sessionId
	);

	void Start();
	void Send(std::string str);
};

class IServer
{
public:
	virtual void Destroy_Session(int sessionId) = 0;
	virtual void Broadcast_Chat(std::string str, int id) = 0;
	virtual int Create_Room(int sessionId, std::string nickname, std::string roomName, std::string roomPassword) = 0;
	virtual std::shared_ptr<Room> Get_Room(int roomId) = 0;
	virtual std::vector<nlohmann::json> Get_RoomJson() = 0;
	virtual void Broadcast_Room_Chat(std::string str, int id, int roomId) = 0;

	virtual ~IServer() = default;
};

class Server : public IServer
{
private:
	unsigned int _nextSessionId = 1;									//다음 부여할 세션 Id
	std::queue<unsigned int> _sessionQueue;								//재사용하기 위해 반환된 세션 Id를 저장할 큐
	std::unordered_map<int, std::shared_ptr<Session>> _sessionMap;		//id를 통해 세션을 찾기 위한 map

	boost::asio::ip::tcp::acceptor _acceptor;
	boost::asio::ssl::context _context;
	boost::asio::strand<boost::asio::io_context::executor_type> _strand;

	//비동기로 Accpet 시작
	void Start_Accept();
	//세션에 부여할 Id 가져오기
	int Get_SessionId();

	unsigned int _nextRoomId = 1;
	std::queue<unsigned int> _roomQueue;
	std::unordered_map<int, std::shared_ptr<Room>> _roomMap;



	//방 제거 (map에서 제거하는거는 아직 안 만듬)
	void Destroy_Room(int roomId);
	
public:
	Server(boost::asio::io_context& io, unsigned short port);


	//Session에서 호출가능한 함수 목록
	//
	//

	//세션 목록에서 세션 제거 후 세션ID 반환
	virtual void Destroy_Session(int sessionId);
	//해당 id를 제외한 전원에게 chat를 전달
	virtual void Broadcast_Chat(std::string str, int id);
	//방 생성
	virtual int Create_Room(int sessionId, std::string nickname, std::string roomName, std::string roomPassword);
	//id를 주면 map에 연결된 Room 포인터를 반환 (콜백함수여서 세션에서 사용)
	virtual std::shared_ptr<Room> Get_Room(int roomId);
	//모든 방의 정보를 백터에 넣어서 반환
	virtual std::vector<nlohmann::json> Get_RoomJson();
	//해당 id를 제외한 방 인원에게 chat를 전달
	virtual void Broadcast_Room_Chat(std::string str, int id, int roomId);

};