#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <functional>
#include <vector>

#include "Gomoku.h"

class Room
{
private:
	//Room끼리 구분하기 위한 int
	int _roomId;
	//Room의 실제 이름
	std::string _roomName;
	//Room의 비밀번호 유무
	bool _isPrivate;
	//Room의 비밀번호
	std::string _roomPassword;
	//Room에 존재하는 Session의 Id 목록
	std::vector<std::pair<int, std::string>> _sessionIdList;
	//Server에게 RoomID 반환 및 Map 요소 삭제
	std::function<void(int)> _destroy_room_callback;




public:
	Room(int roomId, std::string roomName, std::string roomPassword, std::function<void(int)> func);

	//SessionList에 추가
	void AddSessionId(int sessionId, std::string nickname);
	//SessionList에서 삭제 (삭제 후 sessionList가 empty인 경우 room을 서버가 가지고 있는 map에서 삭제)
	void DeleteSessionId(int sessionId);
	//방 비밀번호 대조
	bool VerifyPassword(std::string password);
	//SessionList 가져오기
	const std::vector<std::pair<int, std::string>>* GetSessionIdList();

	nlohmann::json ReturnJson();
};










/*
1. 클라이언트로부터 Room 생성 요청

2. 서버는 숫자를 할당 받아 Room을 생성 후 클라이언트에게 생성 완료된 것을 보고
+ 이때 숫자는 절대 안 겹치게 따로 Manager를 만들어 숫자를 관리하게 할 것 


Manager

방이 생길 때마다 1씩 증가하는 int
방이 삭제될 때마다 큐에 번호를 넣는다

1. 클라이언트가 방번호를 요구
2. 큐 내부에서 반환된 번호를 먼저 확인 (있는 경우 우선적으로 이것을 반환)
3. 큐가 없는 경우 int를 반환하고 증가시킴





3. Room 안에 들어간 클라이언트의 세션에도 _roomId를 저장할 것

4. Room 안에 들어간 클라이언트가 이제 Room관련 작업을 요청할 경우
먼저 Room에 있는지 검사를 하고 수행

5. roomId를 확인하고 roomId에 있는 데이터를 가지고 작업수행

착수를 한 경우 맵을 보고서 유효한 수인지 승패가 갈리는지 확인

6. 완료했으면 Room에 존재하는 모든 소켓에게 브로드캐스트

세션 id로 소켓을 관리하고 있어서 세션 id만 알면 되긴함



방 삭제)

플레이어가 0명인 경우 = 삭제

정상적인 나가기의 경우 카운트가 되지만

비정상적으로 나간 경우도 포함해서 생각할 것


*/