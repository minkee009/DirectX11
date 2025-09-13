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
    this->m_pDevice = m_d3dContext->m_pD3DDevice.Get();
    this->m_pImmediateContext = m_d3dContext->m_pImmediateContext.Get();

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

    ImGui::Begin("Camera State");
    ImGui::Text("Pos X - %f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().x);
    ImGui::Text("Pos Y - %f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().y);
    ImGui::Text("Pos Z - %f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().z);

	Vector3 euler = m_d3dContext->m_pCamera->GetTransform()->GetLocalRotation().ToEuler();

    ImGui::Text("=-= Rot X - %f", XMConvertToDegrees(euler.x));
    ImGui::Text("=-= Rot Y - %f", XMConvertToDegrees(euler.y));
    ImGui::Text("=-= Rot z - %f", XMConvertToDegrees(euler.z));

    static float fov = 75.0f;
    //ImGui::Text("This is some useful text.");
    ImGui::SliderFloat("Field Of View", &fov, 60.0f, 120.0f);
	m_d3dContext->m_pCamera->SetFOV(fov);

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
