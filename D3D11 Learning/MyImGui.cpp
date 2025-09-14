#ifdef _DEBUG

#include "MyImGui.h"
#include "MyD3DContext.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include "Time.h"


bool MyEngine::MyImGui::Initialize(MyD3DContext* myContext)
{
	m_d3dContext = myContext;

    this->m_hWnd = m_d3dContext->m_hWnd;
    this->m_pDevice = m_d3dContext->m_pd3dDevice.Get();
    this->m_pImmediateContext = m_d3dContext->m_pImmediateContext.Get();

    // 코어 컨텍스트 생성
    if (!ImGui::CreateContext()) {
        return false; 
    }
    m_isImGuiInit = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 내비게이션 활성화
	io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesKorean()); // 한글 폰트 설정

    // 스타일 설정
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
	// 둥근 모서리 설정
    style.FrameRounding = 6.0f;
    style.WindowRounding = 10.0f;
	style.GrabRounding = 6.0f;
	style.ScrollbarRounding = 6.0f;

	// 색상 설정
    //style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
    //style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
    //style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 1.00f, 0.01f, 1.00f);
    //style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
    //style.Colors[ImGuiCol_Button] = ImVec4(0.48f, 0.72f, 0.89f, 0.49f);
    //style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
    //style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);

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
    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(260, 340), ImGuiCond_Once);

    ImGui::Begin(u8"카메라 상태", nullptr, ImGuiWindowFlags_NoResize);

    ImGui::Text(u8"마우스 우클릭 -> 카메라 회전 \n마우스 우클릭 + W,A,S,D,Q,E -> 카메라 이동");

    ImGui::NewLine();

    ImGui::Text(u8"위치");
    ImGui::Text("  X : %f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().x);
    ImGui::Text("  Y : %f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().y);
    ImGui::Text("  Z : %f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().z);

    ImGui::NewLine();

	Vector3 euler = m_d3dContext->m_pCamera->GetTransform()->GetLocalRotation().ToEuler();

    ImGui::Text(u8"회전");
    ImGui::Text("  X : %f", XMConvertToDegrees(euler.x));
    ImGui::Text("  Y : %f", XMConvertToDegrees(euler.y));
    ImGui::Text("  z : %f", XMConvertToDegrees(euler.z));

    ImGui::NewLine();

	constexpr float defFov = 75.0f;
    static float fov = defFov;
    ImGui::Text(u8"시야각");
    ImGui::SliderFloat("##FOV", &fov, 60.0f, 120.0f);
	m_d3dContext->m_pCamera->SetFOV(fov);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기화")) {
        fov = defFov;
    }

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

#endif //_DEBUG