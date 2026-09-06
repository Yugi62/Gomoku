#include "Redis.h"
#include <iostream>
#include <unordered_map>

RedisManager::RedisManager()
{
	connection_options.host = "127.0.0.1";							// Redis 서버 주소
	connection_options.port = 6379;									// Redis 포트
	connection_options.socket_timeout = std::chrono::seconds(1);	// 타임아웃 설정
	redis = std::make_unique<sw::redis::Redis>(connection_options);	// Redis 객체 생성
}

void RedisManager::InIt_Player(std::string playerName)
{
	try
	{
		//이미 존재하는 경우 종료
		if (redis->exists(playerName))
			return;

		redis->hset(playerName, "Win", "0");
		redis->hset(playerName, "Loss", "0");
		redis->hset(playerName, "Draw", "0");
		redis->hset(playerName, "Rating", "1000");

		redis->zadd("Ranking", playerName, 1000);
	}
	catch (const sw::redis::Error& err)
	{
		std::cerr << "Redis 오류: " << err.what() << std::endl;
	}
}

void RedisManager::Update_Player(std::string playerName, std::string field, int value)
{
	try
	{
		//존재하지 않는 경우 취소
		if (!redis->exists(playerName))
			return;

		redis->hincrby(playerName, field, value);
	}
	catch (const sw::redis::Error& err)
	{
		std::cerr << "Redis 오류: " << err.what() << std::endl;
	}
}

void RedisManager::Update_Ranking(std::string playerName)
{
	try
	{
		auto temp = redis->hget(playerName, "Rating");
		int rating = std::stoi(*temp);
		redis->zadd("Ranking", playerName, rating);
	}
	catch (const sw::redis::Error& err)
	{
		std::cerr << "Redis 오류: " << err.what() << std::endl;
	}
}

std::vector<std::pair<std::string, double>> RedisManager::Get_Ranking()
{
	std::vector<std::pair<std::string, double>> rankVec;

	try
	{
		redis->zrevrange(
			"Ranking",
			0,
			-1,
			std::back_inserter(rankVec)
		);
	}
	catch (const sw::redis::Error& err)
	{
		std::cerr << "Redis 오류: " << err.what() << std::endl;
	}

	return rankVec;
}

std::unordered_map<std::string, std::string> RedisManager::Get_Player(std::string playerName)
{
	std::unordered_map<std::string, std::string> playerVec;

	try
	{
		redis->hgetall(playerName, std::inserter(playerVec, playerVec.begin()));
	}
	catch (const sw::redis::Error& err)
	{
		std::cerr << "Redis 오류: " << err.what() << std::endl;
	}

	return playerVec;
}