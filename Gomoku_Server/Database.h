#pragma once

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>

class Database
{
private:
	sql::mysql::MySQL_Driver* driver;
	sql::Connection* con;
	bool isInit = false;


private:
	Database() {}
	~Database() {}


public:
	static Database& GetInstance()
	{
		static Database instance;
		return instance;
	}

	void Init();
	//user_id가 db에 존재하는지 확인
	bool CheckUserid(std::string user_id);

	//nickname이 db에 존재하는지 확인
	bool CheckUserNickname(std::string user_nickname);

	//회원가입 한 user를 db에 insert
	void InsertUser(const std::string& user_id, const std::string& user_password, const std::string& user_nickname);

	//user_id로 password를 가져온다 (없는 경우 빈 문자열)
	std::string GetPassword(std::string user_id);

	//user_id로 nickname을 가져온다 (없는 경우 빈 문자열)
	std::string GetNickname(std::string user_id);
};