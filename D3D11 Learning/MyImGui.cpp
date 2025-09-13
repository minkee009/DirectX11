#include "MyImGui.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include "Time.h"


bool MyEngine::MyImGui::Initialize(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext)
{
    this->m_hWnd = hWnd;
    this->m_pDevice = pDevice;
    this->m_pImmediateContext = pImmediateContext;

    // 코어 컨텍스트 생성
    if (!ImGui::CreateContext()) {
        return false; 
    }
    m_isImGuiInit = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 내비게이션 활성화

    // 스타일 설정
    ImGui::StyleColorsDark();

    // Win32 백엔드 초기화
    if (!ImGui_ImplWin32_Init(m_hWnd)) {
        ImGui::DestroyContext();
        return false;
    }
    m_isWin32BackendInit = true;

    // DirectX 11 백엔드 초기화
    if (!ImGui_ImplDX11_Init(m_pDevice, m_pImmediateContext)) {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }
    m_isD3D11BackendInit = true;

    return true;
}

void MyEngine::MyImGui::BeginFrame()
{
    RECT clientRect;
    GetClientRect(m_hWnd,&clientRect);

    // 너비와 높이를 명시적으로 계산
    LONG width = clientRect.right - clientRect.left;
    LONG height = clientRect.bottom - clientRect.top;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height); // float으로 형변환

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void MyEngine::MyImGui::Update()
{
    ImGui::ShowDemoWindow(); // 데모 창 표시 (테스트용)

    RECT clientRect;
    GetClientRect(m_hWnd, &clientRect);

    ImGui::Begin("Hello, ImGui!");
    ImGui::Text("client left: %.2f", (float)clientRect.left);
    ImGui::Text("client right: %.2f", (float)clientRect.right);
    ImGui::Text("client top: %.2f", (float)clientRect.top);
    ImGui::Text("client bottom: %.2f", (float)clientRect.bottom);

    static float f = 0.0f;
    ImGui::Text("This is some useful text.");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::End();
}

void MyEngine::MyImGui::Render()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void MyEngine::MyImGui::Uninitialize()
{
    if(m_isD3D11BackendInit)
        ImGui_ImplDX11_Shutdown();
    if(m_isWin32BackendInit)
        ImGui_ImplWin32_Shutdown();
    if(m_isImGuiInit)
        ImGui::DestroyContext();

    m_isImGuiInit = false;
    m_isWin32BackendInit = false;
    m_isD3D11BackendInit = false;
}
