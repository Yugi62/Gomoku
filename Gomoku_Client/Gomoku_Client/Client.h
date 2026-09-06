#pragma once

#include <queue>
#include <memory>
#include <array>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>

class Client
{
private:
	boost::asio::ssl::context _ssl_context;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> _socket;
	boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> _endpoints;
	boost::asio::strand<boost::asio::io_context::executor_type> _strand;

	std::array<char, 4> _buf;

	boost::mutex _writeMtx;
	std::queue<std::string> _writeQueue;
	bool isWriting = false;

	std::function<void(const std::string&)> _guiCallback;

	void Start_Connect();
	void Start_Handshake();
	void Start_Read();
	void Start_Dispatch(std::string str);

public:
	Client(boost::asio::io_context& io);
	void Reserve_Write(std::string str);
	void Start_Write();
	void Set_Callback(std::function<void(const std::string&)> callback);
};