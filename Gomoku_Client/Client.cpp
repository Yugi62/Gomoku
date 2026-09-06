
#include "Client.h"
#include <nlohmann/json.hpp>

Client::Client(boost::asio::io_context& io) try :
	_ssl_context(boost::asio::ssl::context::tlsv12_client),
	_socket(io, _ssl_context),
	_strand(boost::asio::make_strand(io))
{
	//인증서 검증 방식 (실제 인증서를 발급받으면 verify_peer로 변경)
	_ssl_context.set_verify_mode(boost::asio::ssl::verify_none);

	//소켓 생성 및 엔드포인트 설정
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket(io, _ssl_context);
	boost::asio::ip::tcp::resolver resolver(io);
	_endpoints = resolver.resolve("127.0.0.1", "6799");

	//비동기로 Conenct
	Start_Connect();
}
catch (const std::exception& e)
{
	//std::cout << "Client Error : " << e.what() << std::endl;
}

void Client::Start_Connect()
{
	boost::asio::async_connect(
		_socket.lowest_layer(),
		_endpoints,
		[this](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endppoint) 
		{		
			//Connect 성공 시 비동기로 핸드쉐이크 시작
			if (!error)
			{
				Start_Handshake();
			}
		}
	);
}

void Client::Start_Handshake()
{
	_socket.async_handshake(
	boost::asio::ssl::stream_base::client,
		[this](const boost::system::error_code& error)
		{
			if (!error)
			{
				Start_Read();
			}
		}	
	);
}

void Client::Start_Read()
{
	//글자 수 읽기 (4바이트)
	_socket.async_read_some(boost::asio::buffer(_buf),
		[this](const boost::system::error_code& error, std::size_t len1)
		{
			if (!error)
			{
				//쓰레기 값이 들어갈 수 있으므로 string으로 변환 시 딱 len만큼만 넣는다
				std::string str1(_buf.data(), len1);

				//구한 크기를 정수로 파싱
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
							std::string str(dataBuf->data(), len2);

							//받은 string을 처리
							Start_Dispatch(str);
						}
					});
			}
		}
	);
}

void Client::Reserve_Write(std::string str)
{
	//push·pop이 서로 다른 스레드에서 진행되므로 동기화
	_writeMtx.lock();
	_writeQueue.push(str);
	_writeMtx.unlock();

	if (!isWriting)
		Start_Write();
}


void Client::Start_Write()
{
	isWriting = true;

	//큐 작업을 위한 뮤텍스 처리 (return이 도중에 되더라도 unlock되게 unique_lock을 사용)
	boost::unique_lock<boost::mutex> lock(_writeMtx);

	if (_writeQueue.empty())
	{
		isWriting = false;
		return;
	}
	std::string str = _writeQueue.front();
	_writeQueue.pop();

	//큐 작업 완료 후 unlock
	lock.unlock();

	//비동기 작업이 완료될 때까지 버퍼가 살아있어야하므로 스마트 포인터로 초기화
	auto buffer = std::make_shared<std::string>(str);

	_socket.async_write_some(boost::asio::buffer(*buffer),
		[this, buffer](const boost::system::error_code& error, std::size_t len)
		{
				if (!error)
				{
					//write 재호출 (만약 큐가 비었을 경우 그대로 종료)
					Start_Write();
				}
		}
	);
}

void Client::Start_Dispatch(std::string str)
{
	_guiCallback(str);
}

void Client::Set_Callback(std::function<void(const std::string&)> callback)
{
	_guiCallback = callback;
}