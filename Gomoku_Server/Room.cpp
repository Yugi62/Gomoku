#include "Room.h"
#include <algorithm>


Room::Room(int roomId, std::string roomName, std::string roomPassword, std::function<void(int)> func)
	: _roomId(roomId), _roomName(roomName)
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
	_sessionIdList.push_back(std::make_pair(sessionId, nickname));
}

void Room::DeleteSessionId(int sessionId)
{
	//삭제 후 vector가 빈 경우 room을 삭제
	//map에서도 삭제해야함
	//id도 반환해야하고

	auto it = find_if(_sessionIdList.begin(), _sessionIdList.end(),
		[&sessionId](auto& p) { return p.first == sessionId; });

	if (it != _sessionIdList.end())
		_sessionIdList.erase(it);

	//세션이 빈 경우 서버에게 방 삭제를 요청 (map에서 삭제되면 참조 카운터가 0이 되면서 삭제)
	if (_sessionIdList.empty())
		_destroy_room_callback(_roomId);
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

std::vector<std::pair<int, std::string>>* Room::GetSessionIdList()
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
