#pragma once

#include "Client.h"
#include "Gomoku.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <unordered_map>
#include <functional>

#include <nlohmann/json.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Gui
{
private:    
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    IDXGISwapChain* g_pSwapChain = nullptr;
    bool                     g_SwapChainOccluded = false;
    UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
    HWND hwnd;
    WNDCLASSEXW wc;

    Client& _client;
    ImGuiWindowFlags defaultFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    bool isLogin = true;
    bool isLoginLoading = false;
    bool isSend = false;
    bool isRegister = false;
    bool isRegisterLoading = false;

    std::string isUsernameExistMsg = "";
    bool isUsernameAvaliable = false;

    std::string isNicknameExistMsg = "";
    bool isNicknameAvaliable = false;

    bool isLobby = false;
    std::vector<std::string> chatMsg;

    bool isRanking = false;

    std::string nickname;

    Gomoku gomoku;

    bool isRoom = false;

    std::vector<nlohmann::json> roomVec;
    std::vector<std::string> nicknames;

    std::unordered_map<std::string, std::function<void(nlohmann::json&)>> dispatch_Map;

    nlohmann::json rankJson;


private:
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Cleanup();

    void Draw_Login();
    void Draw_Register();
    void Draw_Lobby();
    void Draw_Ranking();
    void Draw_Gomoku();

    void Check_Username(nlohmann::json& j);
    void Check_UserNickname(nlohmann::json& j);
    void Register(nlohmann::json& j);
    void Login(nlohmann::json& j);
    void Chat(nlohmann::json& j);
    void Create_Room(nlohmann::json& j);
    void Refresh_Room(nlohmann::json& j);
    void Join_Room(nlohmann::json& j);
    void Room_Chat(nlohmann::json& j);
    void Refresh_PlayerInfo(nlohmann::json& j);
    void Start_Game(nlohmann::json& j);
    void Place_Stone(nlohmann::json& j);
    void Reset_Board(nlohmann::json& j);
    void Send_Ranking(nlohmann::json& j);

public:
    Gui(Client& client);
    void Dispatch_Data(std::string str);
    void Run();
};