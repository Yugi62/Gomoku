#pragma once

#include "Gui.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <nlohmann/json.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::string fillZero(std::string str, int width)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(width) << str;
    str = oss.str();
    return str;
}

bool Gui::CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void Gui::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void Gui::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void Gui::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

LRESULT WINAPI Gui::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Gui* pThis;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<Gui*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else
        pThis = reinterpret_cast<Gui*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        pThis->g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        pThis->g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void Gui::Cleanup()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

void Gui::Draw_Login()
{
    //로그인 (최초 윈도우)
    if (isLogin)
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float width = displaySize.x;
        float height = displaySize.y;
        ImGui::SetNextWindowPos(ImVec2(width * 0.05f, height * 0.05f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(width * 0.9f, height * 0.9f), ImGuiCond_Once);

        ImGui::Begin("Login", nullptr, defaultFlags);

        if (!isLoginLoading)
        {
            static char id[128] = "";
            ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
            static char password[128] = "";
            ImGui::InputText("PASSWORD", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);

            if (ImGui::Button("Login"))
            {
                isLoginLoading = true;

                nlohmann::json j;

                j["type"] = "Login";
                j["id"] = id;
                j["password"] = password;

                std::string str = j.dump();
                str = fillZero(std::to_string(str.size()), 4) + str;
                _client.Reserve_Write(str);
            }

            if (ImGui::Button("Register"))
                isRegister = true;
        }

        ImGui::End();
    }

    if (isSend)
    {
        ImGui::OpenPopup("Login Error");
        isSend = false;
    }
    if (ImGui::BeginPopupModal("Login Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Login Error");

        if (ImGui::Button("OK"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void Gui::Draw_Register()
{

    if (!isRegisterLoading)
    {
        if (isRegister)
        {
            //회원가입
            ImGui::Begin("Register");

            static char id[128] = "";
            ImGui::InputText("ID", id, IM_ARRAYSIZE(id));

            //유저 ID 유효성 검사
            if (ImGui::Button("Check ID Availability"))
            {
                nlohmann::json j;

                j["type"] = "Check_Username";
                j["id"] = id;
                std::string str = j.dump();
                str = fillZero(std::to_string(str.size()), 4) + str;
                _client.Reserve_Write(str);
            }
            ImGui::Text(isUsernameExistMsg.c_str());

            static char password[128] = "";
            ImGui::InputText("Password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);

            static char nickname[128] = "";
            ImGui::InputText("Nickname", nickname, IM_ARRAYSIZE(nickname));

            //유저 ID 유효성 검사
            if (ImGui::Button("Check Nickname Availability"))
            {
                nlohmann::json j;

                j["type"] = "Check_UserNickname";
                j["nickname"] = nickname;
                std::string str = j.dump();
                str = fillZero(std::to_string(str.size()), 4) + str;
                _client.Reserve_Write(str);
            }
            ImGui::Text(isNicknameExistMsg.c_str());


            if (ImGui::Button("Register"))
            {
                if (isUsernameAvaliable && isNicknameAvaliable)
                {
                    //서버로부터 데이터를 받을 때까지 대기 
                    isRegisterLoading = true;

                    nlohmann::json j;

                    j["type"] = "Register";
                    j["id"] = id;
                    j["password"] = password;
                    j["nickname"] = nickname;
                    std::string str = j.dump();
                    str = fillZero(std::to_string(str.size()), 4) + str;
                    _client.Reserve_Write(str);
                }
                else
                {
                    ImGui::OpenPopup("Availability Error");
                }
            }

            if (ImGui::BeginPopupModal("Availability Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Check Availability");

                if (ImGui::Button("OK"))
                    ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }

            ImGui::End();
        }
    }
    else
    {

    }


}

void Gui::Draw_Lobby()
{
    if (isLobby)
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float width = displaySize.x;
        float height = displaySize.y;
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);

        ImGui::Begin("Lobby", nullptr, defaultFlags);

        //방 생성 팝업
        if (ImGui::Button("Create Room"))
        {
            ImGui::OpenPopup("CreateRoom_Popup");
        }
        if (ImGui::BeginPopupModal("CreateRoom_Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char roomName[64] = "";
            ImGui::InputText("RoomName", roomName, IM_ARRAYSIZE(roomName));

            static char password[64] = "";
            ImGui::InputText("Password", password, IM_ARRAYSIZE(password));

            if (ImGui::Button("Ok"))
            {
                nlohmann::json j;
                j["type"] = "Create_Room";
                j["name"] = roomName;
                j["password"] = password;
                j["nickname"] = nickname;
                std::string str = j.dump();
                str = fillZero(std::to_string(str.size()), 4) + str;

                _client.Reserve_Write(str);

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        //뒤로가기
        if (ImGui::Button("Back"))
        {

        }

        ImGui::SameLine();

        static int selected = -1;
        if (ImGui::Button("Refresh"))
        {
            selected = -1;
            roomVec.clear();

            nlohmann::json j;
            j["type"] = "Refresh_Room";
            std::string str = j.dump();
            str = fillZero(std::to_string(str.size()), 4) + str;
            _client.Reserve_Write(str);
        }

        ImGui::SameLine();

        if (ImGui::Button("Ranking"))
        {
            nlohmann::json j;
            j["type"] = "Send_Ranking";
            std::string str = j.dump();
            str = fillZero(std::to_string(str.size()), 4) + str;
            _client.Reserve_Write(str);
        }

        ImGui::Separator();

        // 스크롤 가능한 방 목록
        ImGui::BeginChild("RoomListChild", ImVec2(0, 200), true);

        for (int i = 0; i < roomVec.size(); i++)
        {
            std::string str = roomVec[i]["roomName"];

            if (ImGui::Selectable(str.c_str(), selected == i))
            {
                selected = i;
            }
        }
        ImGui::EndChild();

        static char pass[128] = "";
        ImGui::InputText("Password", pass, IM_ARRAYSIZE(pass));

        ImGui::SameLine();

        if (ImGui::Button("Join"))
        {
            //유효한 인덱스인지 확인
            if (selected >= 0 && selected < roomVec.size())
            {
                nlohmann::json j;
                j["type"] = "Join_Room";
                j["roomId"] = roomVec[selected]["roomId"];
                j["password"] = pass;
                j["nickname"] = nickname;

                std::string str = j.dump();
                str = fillZero(std::to_string(str.size()), 4) + str;
                _client.Reserve_Write(str);
            }
        }

        ImGui::End();

        //방 생성 성공 시
        if (isRoom)        
            Draw_Gomoku();        
    }
}

void Gui::Draw_Ranking()
{
    if (isRanking)
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float width = displaySize.x;
        float height = displaySize.y;
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);

        ImGui::Begin("Ranking", nullptr, defaultFlags);

        struct RankInfo
        {
            std::string name;
            int win;
            int lose;
            int draw;
            int rating;
        };

        ImGui::BeginChild("RankingScroll", ImVec2(0, 400), true);

        /*
        ImGuiTableFlags_Borders : 테이블에 테두리를 그려줌
        ImGuiTableFlags_RowBg : 행마다 배경 색을 번갈아 적용
        ImGuiTableFlags_SizingStretchProp : 컬럼 너비를 자동 조정
        */

        if (ImGui::BeginTable("RankingTable", 6,
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Rank");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Win");
            ImGui::TableSetupColumn("Lose");
            ImGui::TableSetupColumn("Draw");
            ImGui::TableSetupColumn("Rating");

            //컬럼명 보여주기
            ImGui::TableHeadersRow();

            std::string ss = rankJson["data"][0]["Name"];

            for (auto& player : rankJson["data"])
            {
                //데이터 행의 시작을 선언
                ImGui::TableNextRow();

                // Rank
                ImGui::TableSetColumnIndex(0);
                int rank = player["Rank"];
                ImGui::Text("%d", rank);

                // Name
                ImGui::TableSetColumnIndex(1);
                std::string name = player["Name"];
                ImGui::Text("%s", name.c_str());

                // Win
                ImGui::TableSetColumnIndex(2);
                int win = player["Win"];
                ImGui::Text("%d", win);

                // Lose
                ImGui::TableSetColumnIndex(3);
                int loss = player["Loss"];
                ImGui::Text("%d", loss);

                // Draw
                ImGui::TableSetColumnIndex(4);
                int draw = player["Draw"];
                ImGui::Text("%d", draw);

                // Rating
                ImGui::TableSetColumnIndex(5);
                int rating = player["Rating"];
                ImGui::Text("%d", rating);
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        // 공간 확보 (버튼을 아래로)
        ImGui::Dummy(ImVec2(0, 10));

        // 오른쪽 정렬 핵심
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float buttonWidth = 100.0f;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + windowWidth - buttonWidth);

        if (ImGui::Button("Exit", ImVec2(buttonWidth, 30)))
        {
            isRanking = false;
        }

        ImGui::End();
    }
}

void Gui::Draw_Gomoku()
{
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float width = displaySize.x;
    float height = displaySize.y;
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);

    ImGui::Begin("Gomoku Board", nullptr , defaultFlags);


    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.55f, 0.40f, 0.25f, 1.0f));
    ImGui::BeginChild("Board", ImVec2(500, 500), true);

    const int BOARD_SIZE = 15;
    gomoku.CreateBoard(BOARD_SIZE);

    //30.0f만큼 오른쪽으로
    ImGui::Dummy(ImVec2(30.0f, 0.0f));
    ImGui::SameLine();

    float cellSize = 30.0f; // 한 칸 크기
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float snapSize = cellSize * 0.5f; //최대가 0.5f (더 크게 하면 안 됨)

    //윈도우를 기준으로 현 마우스 위치의 좌표를 반환
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    //윈도우를 기준으로 UI의 맨 왼쪽 위의 좌표를 반환
    ImVec2 origin = ImGui::GetCursorScreenPos();


    if (gomoku.ShowTurn())
    {
        //클릭 시 보드에 돌 착수
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            //마우스 좌표를 로컬 좌표로 변경 (마우스 좌표 - UI의 맨 왼쪽 위의 좌표)
            ImVec2 local = ImVec2(mousePos.x - origin.x, mousePos.y - origin.y);

            //보정을 안하면 89,89를 누르면 바로 옆에 있어도 3,3이 아닌 2,2로 인식되서 snapSize를 더한다
            // +로컬 좌표로 89,89면 한 칸 당 30이니깐 3,3 근처이다
            int x = (int)((local.x + snapSize) / cellSize);
            int y = (int)((local.y + snapSize) / cellSize);

            if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE)
            {
                gomoku.SetBoard(y, x, gomoku.ShowColor());

                gomoku.SetTurn(false);

                nlohmann::json j;
                j["type"] = "Place_Stone";
                j["x"] = x;
                j["y"] = y;
                std::string str = j.dump();
                str = fillZero(std::to_string(str.size()), 4) + str;
                _client.Reserve_Write(str);
            }
        }
    }

    //격자 그리기
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        // 가로줄
        draw_list->AddLine(
            ImVec2(origin.x, origin.y + i * cellSize),
            ImVec2(origin.x + (BOARD_SIZE - 1) * cellSize, origin.y + i * cellSize),
            IM_COL32(0, 0, 0, 255)
        );
        // 세로줄
        draw_list->AddLine(
            ImVec2(origin.x + i * cellSize, origin.y),
            ImVec2(origin.x + i * cellSize, origin.y + (BOARD_SIZE - 1) * cellSize),
            IM_COL32(0, 0, 0, 255)
        );
    }

    //돌 그리기
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (gomoku.ShowBoard()[y][x] == 0)
                continue;

            ImU32 color;
            if (gomoku.ShowBoard()[y][x] == 1)
                color = IM_COL32(0, 0, 0, 255);         //흑돌

            else if (gomoku.ShowBoard()[y][x] == 2)
                color = IM_COL32(255, 255, 255, 255);   // 백돌

            ImVec2 center = ImVec2
            (
                origin.x + x * cellSize,
                origin.y + y * cellSize
            );
            draw_list->AddCircleFilled(center, cellSize * 0.4f, color);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::BeginChild("UI", ImVec2(200, 500), true);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::BeginChild("Top", ImVec2(0, 120), true);

    std::string p1 = "Player 1 : ";

    if (!nicknames.empty())
        p1 += nicknames[0];

    std::string p2 = "Player 2 : ";

    if (nicknames.size() >= 2)
        p2 += nicknames[1];

    ImGui::Text(p1.c_str());
    ImGui::Text(p2.c_str());


    if (ImGui::Button("Ready"))
    {
        nlohmann::json j;
        j["type"] = "Room_Ready";
        std::string str = j.dump();
        str = fillZero(std::to_string(str.size()), 4) + str;
        _client.Reserve_Write(str);
    }

    if (gomoku.ShowGameStatus())
    {
        ImDrawList* draw_listt = ImGui::GetWindowDrawList();


        std::string turnStr;
        turnStr = gomoku.ShowTurn() ? "My Turn" : "Opponent's Turn";

        ImU32 stoneColor;

        if (gomoku.ShowTurn())
        {
            stoneColor = gomoku.ShowColor() == 1 ? IM_COL32(0, 0, 0, 255) : IM_COL32(255, 255, 255, 255);
        }
        else
        {
            //내거의 반대를 가져와야함 1이면 2 2이면 1

            if (gomoku.ShowColor() == 1)
                stoneColor = IM_COL32(255, 255, 255, 255);

            else if (gomoku.ShowColor() == 2)
                stoneColor = IM_COL32(0, 0, 0, 255);
        }

        ImVec2 post = ImGui::GetCursorScreenPos();

        draw_listt->AddCircleFilled(
            ImVec2(post.x + 10, post.y + 7),
            6.0f,
            stoneColor
        );

        // 글자 그리기
        draw_listt->AddText(
            ImVec2(post.x + 30, post.y),
            IM_COL32_WHITE,
            turnStr.c_str());
    }


    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::BeginChild("Bottom", ImVec2(0, 240), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (auto msg : chatMsg)
    {
        ImGui::TextWrapped("%s", msg.c_str());
    }

    // 자동 스크롤 맨 아래
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    static char chat[128] = "";
    ImGui::InputText("Chat", chat, IM_ARRAYSIZE(chat));

    if (ImGui::Button("Enter"))
    {
        std::string temp = nickname + " : " + chat;
        chatMsg.push_back(temp);

        nlohmann::json j;
        j["type"] = "Room_Chat";
        j["chat"] = temp;
        std::string str = j.dump();
        str = fillZero(std::to_string(str.size()), 4) + str;
        _client.Reserve_Write(str);
    }

    if (ImGui::Button("Back"))
    {
        chatMsg.clear();
        nicknames.clear();

        nlohmann::json j;
        j["type"] = "Exit_Room";
        std::string str = j.dump();
        str = fillZero(std::to_string(str.size()), 4) + str;
        _client.Reserve_Write(str);

        isRoom = false;
    }


    ImGui::EndChild();

    ImGui::End();
}

Gui::Gui(Client& client) : _client(client)
{
    defaultFlags |= ImGuiWindowFlags_NoCollapse;
    defaultFlags != ImGuiWindowFlags_NoResize;

    dispatch_Map["Check_Username"] = [this](nlohmann::json& j) {Check_Username(j); };
    dispatch_Map["Check_UserNickname"] = [this](nlohmann::json& j) {Check_UserNickname(j); };
    dispatch_Map["Register"] = [this](nlohmann::json& j) {Register(j); };
    dispatch_Map["Login"] = [this](nlohmann::json& j) {Login(j); };
    dispatch_Map["Chat"] = [this](nlohmann::json& j) {Chat(j); };
    dispatch_Map["Create_Room"] = [this](nlohmann::json& j) {Create_Room(j); };
    dispatch_Map["Refresh_Room"] = [this](nlohmann::json& j) {Refresh_Room(j); };
    dispatch_Map["Join_Room"] = [this](nlohmann::json& j) {Join_Room(j); };
    dispatch_Map["Room_Chat"] = [this](nlohmann::json& j) {Room_Chat(j); };
    dispatch_Map["Refresh_PlayerInfo"] = [this](nlohmann::json& j) {Refresh_PlayerInfo(j); };
    dispatch_Map["Game_Start"] = [this](nlohmann::json& j) {Start_Game(j); };
    dispatch_Map["Place_Stone"] = [this](nlohmann::json& j) {Place_Stone(j); };
    dispatch_Map["Reset_Board"] = [this](nlohmann::json& j) {Reset_Board(j); };
    dispatch_Map["Send_Ranking"] = [this](nlohmann::json& j) {Send_Ranking(j); };
}

void Gui::Run()
{
    wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    hwnd = ::CreateWindowW(wc.lpszClassName, L"Gomoku Client", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, 100, 100, 800, 600, nullptr, nullptr, wc.hInstance, this);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls


     



    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 메인 루프
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        /*
        여기부터 UI 그리기
        */

        //로그인
        Draw_Login();        
        //회원가입
        Draw_Register();
        //로비
        Draw_Lobby();
        //랭킹
        Draw_Ranking();

        //////////////////////

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
    Cleanup();
}

void Gui::Dispatch_Data(std::string str)
{
    nlohmann::json j;
    j = nlohmann::json::parse(str);

    if (dispatch_Map.find(j["type"]) != dispatch_Map.end())
        dispatch_Map[j["type"]](j);
}

void Gui::Check_Username(nlohmann::json& j)
{
    if (j["result"] == "True")
    {
        isUsernameExistMsg = "Username already exists";
        isUsernameAvaliable = false;
    }
    else if (j["result"] == "False")
    {
        isUsernameExistMsg = "Username is available";
        isUsernameAvaliable = true;
    }
}

void Gui::Check_UserNickname(nlohmann::json& j)
{
    if (j["result"] == "True")
    {
        isNicknameExistMsg = "Nickname already exists";
        isNicknameAvaliable = false;
    }
    else if (j["result"] == "False")
    {
        isNicknameExistMsg = "Nickname is available";
        isNicknameAvaliable = true;
    }

}

void Gui::Register(nlohmann::json& j)
{
    isRegisterLoading = false;

    //회원가입 성공
    if (j["result"] == "True")
    {


    }
    //회원가입 실패
    else if (j["result"] == "False")
    {

    }
}

void Gui::Login(nlohmann::json & j)
{
    isLoginLoading = false;

    //로그인 성공
    if (j["result"] == "True")
    {
        nickname = j["nickname"];

        if (!isLobby)
        {
            isLogin = false;
            isLobby = true;
        }
    }
    //로그인 실패
    else if (j["result"] == "False")
    {
        isSend = true;
    }
}

void Gui::Chat(nlohmann::json& j)
{
    chatMsg.push_back(j["chat"]);
}

void Gui::Create_Room(nlohmann::json& j)
{
    if (j["result"] == "True")
    {
        isRoom = true;
    }
}

void Gui::Refresh_Room(nlohmann::json& j)
{
    for (auto& j : j["rooms"])
        roomVec.push_back(j);
}

void Gui::Join_Room(nlohmann::json& j)
{
    if (j["result"] == "True")
    {
        isRoom = true;
    }
}

void Gui::Room_Chat(nlohmann::json& j)
{
    chatMsg.push_back(j["chat"]);
}

void Gui::Refresh_PlayerInfo(nlohmann::json& j)
{
    nicknames.clear();

    for (auto& v : j["nicknames"])
        nicknames.push_back(v);
}

void Gui::Start_Game(nlohmann::json& j)
{
    chatMsg.push_back("Server : Game Started");

    gomoku.StartGame();

    if (j["isMyTurn"] == "True")
    {
        gomoku.SetTurn(true);
        gomoku.SetColor(true);
    }
    else if (j["isMyTurn"] == "False")
    {
        gomoku.SetTurn(false);
        gomoku.SetColor(false);
    }
}

void Gui::Place_Stone(nlohmann::json& j)
{
    int x = j["x"];
    int y = j["y"];
    int color = j["color"];
    gomoku.SetBoard(y, x, color);
    gomoku.SetTurn(true);
}

void Gui::Reset_Board(nlohmann::json& j)
{
    gomoku.EndGame();
}

void Gui::Send_Ranking(nlohmann::json& j)
{
    rankJson = j;

    std::string ss = rankJson["data"][0]["Name"];
    isRanking = true;
}