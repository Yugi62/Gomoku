#include <sw/redis++/redis++.h>
#include <iostream>
#include <vector>
#include <thread>
#include "Server.h"
#include "Database.h"

int main()
{
	//데이터베이스 싱글톤으로 초기화
	Database::GetInstance().Init();

	//서버 클래스 초기화
	boost::asio::io_context io;
	Server server(io, 6799);

	//CPU 개수만큼 스레드 생성 후 내부에서 run 함수 호출 (=핸들러의 처리를 멀티 스레드로 하기 위해)
	std::vector<std::thread> threads;
	for (int i = 0; i < std::thread::hardware_concurrency(); i++)
		threads.emplace_back([&io]() {io.run(); });

	//모든 스레드 종료될 때까지 대기
	for (auto& t : threads)
		t.join();	
			
	return 0;
}