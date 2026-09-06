#pragma once

#include <sw/redis++/redis++.h>
#include <memory>

class RedisManager
{
private:
	sw::redis::ConnectionOptions connection_options;
	std::unique_ptr<sw::redis::Redis> redis;

public:
	RedisManager();
	void InIt_Player(std::string playerName);									//최초 플레이어 생성 시 호출
	void Update_Player(std::string playerName, std::string field, int value);	//플레이어 정보 업데이트
	void Update_Ranking(std::string playerName);								//랭킹 갱신
	std::vector<std::pair<std::string, double>> Get_Ranking();
	std::unordered_map<std::string, std::string> Get_Player(std::string playerName);
};