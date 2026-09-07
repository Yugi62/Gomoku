### Gomoku
---

Gomoku는 온라인 멀티 오목 서버입니다

+ TCP/SSL로 통신
+ MySQL로 로그인 기능 구현 (비밀번호 Hash + Salt 처리)
+ REDIS로 랭킹 기능 구현
+ Boost.IOCP 기반의 비동기 Overlapped I/O
+ 멀티스레드 기반 서버 처리
+ 방 생성 기능을 통해 여러 명의 오목 플레이가 가능 

### 기술스택
---
+ C++ (서버 및 클라이언트 개발)
+ Boost.Asio (비동기 서버 구현)
+ MySQL (회원 및 데이터 저장)
+ REDIS (랭킹 데이터 관리)
+ ImGui (클라이언트 구현)

### 동작과정
---
<img width="1536" height="1024" alt="Image" src="https://github.com/user-attachments/assets/1ffb12e3-55a7-407e-8fb9-2c7c0f051028" />

### 실제 플레이
---

<img width="400" height="285" alt="Image" src="https://github.com/user-attachments/assets/88aebb1c-9ab4-430e-bf36-6de58aa9513c" />
