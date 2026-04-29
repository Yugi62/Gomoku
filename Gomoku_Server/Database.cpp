#include "Database.h"
#include "utility.h"

void Database::Init()
{
	if (!isInit)
	{
		try
		{
			driver = sql::mysql::get_mysql_driver_instance();
			con = driver->connect("tcp://127.0.0.1:3306", "root", "0000");
			con->setSchema("game");
		}
		catch (sql::SQLException& error)
		{
			//cout << e.what() << endl;
		}
	}
}

bool Database::CheckUserid(std::string user_id)
{
	//파라미터 바인딩 (sql injection 방지)
	sql::PreparedStatement* ps = con->prepareStatement("select user_id from users where user_id = ?");
	ps->setString(1, user_id);
	auto result = ps->executeQuery();

	std::string str = "";
	while (result->next())
		str = result->getString("user_id");

	if (str == "")
		return false;
	else
		return true;
}

bool Database::CheckUserNickname(std::string user_nickname)
{
	//파라미터 바인딩 (sql injection 방지)
	sql::PreparedStatement* ps = con->prepareStatement("select user_nickname from users where user_nickname = ?");
	ps->setString(1, user_nickname);
	auto result = ps->executeQuery();

	std::string str = "";
	while (result->next())
		str = result->getString("user_nickname");

	if (str == "")
		return false;
	else
		return true;
}

void Database::InsertUser(const std::string& user_id, const std::string& user_password, const std::string& user_nickname)
{
	//insert 전 비밀번호 해싱
	std::string hashedPassword = Utility::hashPassword(user_password);

	//파라미터 바인딩 (sql injection 방지)
	sql::PreparedStatement* ps = con->prepareStatement("insert into users values (default, ?, ?, ?, default)");
	ps->setString(1, user_id);
	ps->setString(2, hashedPassword);
	ps->setString(3, user_nickname);

	auto result = ps->executeQuery();
}

std::string Database::GetPassword(std::string user_id)
{
	//파라미터 바인딩 (sql injection 방지)
	sql::PreparedStatement* ps = con->prepareStatement("select user_password from users where user_id = ?");
	ps->setString(1, user_id);
	auto result = ps->executeQuery();

	std::string str = "";

	while (result->next())
		str = result->getString("user_password");

	return str;
}

std::string Database::GetNickname(std::string user_id)
{
	//파라미터 바인딩 (sql injection 방지)
	sql::PreparedStatement* ps = con->prepareStatement("select user_nickname from users where user_id = ?");
	ps->setString(1, user_id);
	auto result = ps->executeQuery();

	std::string str = "";

	while (result->next())
		str = result->getString("user_nickname");

	return str;
}