#ifdef _DEBUG

#include "MyImGui.h"
#include "MyD3DContext.h"

#include <string>
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
    GetClientRect(m_hWnd, &clientRect);

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
    ImGui::SetNextWindowSize(ImVec2(260, 380), ImGuiCond_Once);

    ImGui::Begin(u8"카메라 상태", nullptr, ImGuiWindowFlags_NoResize);

    ImGui::Text(u8"마우스 우클릭 -> 카메라 회전 \n마우스 우클릭 + W,A,S,D,Q,E -> 카메라 이동");

    ImGui::Separator();

    ImGui::Text(u8"위치");
    ImGui::Text("  X : %.3f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().x);
    ImGui::Text("  Y : %.3f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().y);
    ImGui::Text("  Z : %.3f", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().z);

    Vector3 euler = m_d3dContext->m_pCamera->GetTransform()->GetLocalRotation().ToEuler();

    ImGui::Text(u8"회전");
    ImGui::Text("  X : %.3f", XMConvertToDegrees(euler.x));
    ImGui::Text("  Y : %.3f", XMConvertToDegrees(euler.y));
    ImGui::Text("  z : %.3f", XMConvertToDegrees(euler.z));

    ImGui::Separator();

    constexpr float defFov = 75.0f;
    static float fov = defFov;
    ImGui::Text(u8"시야각");
    ImGui::SliderFloat("##FOV", &fov, 60.0f, 120.0f);
    m_d3dContext->m_pCamera->SetFOV(fov);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기화")) {
        fov = defFov;
    }

    constexpr float defNearPlane = 0.3f;
    static float nearPlane = 0.3f;
    ImGui::Text("Near Plane");
    ImGui::SliderFloat("##Near", &nearPlane, 0.01f, 25.0f, "%.2f");
    m_d3dContext->m_pCamera->SetNearPlane(nearPlane);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기화##2")) {
        nearPlane = defNearPlane;
    }

    constexpr float defFarPlane = 1000.0f;
    static float farPlane = 1000.0f;
    ImGui::Text("Far Plane");
    ImGui::SliderFloat("##Far", &farPlane, 0.02f, 1000.0f, "%.2f");
    m_d3dContext->m_pCamera->SetFarPlane(farPlane);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기화##3")) {
        farPlane = defFarPlane;
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(5, 390), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(220, 374), ImGuiCond_Once);

    ImGui::Begin(u8"오브젝트 상태", nullptr, ImGuiWindowFlags_NoResize);

    auto obj1 = m_d3dContext->m_sceneObjects[0].get();

    constexpr Vector3 obj1_defpos = { 0.0f, 0.0f, 0.0f };
    constexpr Vector3 obj2_defpos = { 8.0f, 0.0f, 0.0f };
    constexpr Vector3 obj3_defpos = { 4.0f, 0.0f, 0.0f };

    static Vector3 obj1_pos = { 0.0f, 0.0f, 0.0f };
    static Vector3 obj2_pos = { 8.0f, 0.0f, 0.0f };
    static Vector3 obj3_pos = { 4.0f, 0.0f, 0.0f };

    ImGui::Text(u8"오브젝트 1 월드 위치");
    if (ImGui::DragFloat3("##obj1_pos", &obj1_pos.x,0.05f))
    {
        obj1->SetLocalPosition(obj1_pos);
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        obj1->SetLocalPosition(obj1_pos);
    }
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값")) {
        obj1_pos = obj1_defpos;
        obj1->SetLocalPosition(obj1_defpos);
    }

    ImGui::Text(u8"오브젝트 1 회전 값 (오일러)");
    constexpr Vector3 obj1_defEulerRot = { 0,0,0 };
    static Vector3 obj1_rot = obj1->GetLocalEulerRotation();
    if (ImGui::DragFloat3("##obj1_rot", &obj1_rot.x,0.1f))
    {
        obj1->SetLocalEulerRotation(obj1_rot);
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        obj1->SetLocalEulerRotation(obj1_rot);
    }
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##2")) {
        obj1_rot = obj1_defEulerRot;
        obj1->SetLocalEulerRotation(obj1_defEulerRot);
    }

    ImGui::Text(u8"빛");

    ImGui::ColorEdit3("##Light1Color", &m_d3dContext->m_lightColors[0].x);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##3")) {
        m_d3dContext->m_lightColors[0] = { 1,1,1,1 };
    }

    constexpr Vector3 light1_defEulerRot = { 0,0,0 };
    static Vector3 light1_rot = { 0,0,0 };
    ImGui::DragFloat3("##Light1Dir", &light1_rot.x, 0.1f);
    auto light1_angleRot = Vector3{ XMConvertToRadians(light1_rot.x),XMConvertToRadians(light1_rot.z) ,XMConvertToRadians(light1_rot.y) };
    auto light1_dir = Vector3::Transform({ 0,1,0 }, Quaternion::CreateFromYawPitchRoll(light1_angleRot.y, light1_angleRot.x, light1_angleRot.z));
    m_d3dContext->m_lightDirs[0] = { light1_dir.x, light1_dir.y, light1_dir.z, 1 };
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##4")) {
        light1_rot = light1_defEulerRot;
        light1_dir = Vector3::Transform({ 0,1,0 }, Quaternion::CreateFromYawPitchRoll(light1_rot.y, light1_rot.x, light1_rot.z));
        m_d3dContext->m_lightDirs[0] = { 1,0,0,1 };
    }


    ImGui::SliderFloat("##LightDist", &m_d3dContext->m_lightDistance, 0.0f, 5.0f);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##5")) {
        m_d3dContext->m_lightDistance = 5.0f;
    }

    //cb.diffuseStr = m_diffuseStrength;
    //cb.specularStr = m_specularStrength;
    //cb.shininess = m_shininess;

    ImGui::Text(u8"환경광(ambient) : 색");
    ImGui::ColorEdit3("##AmbientColor", &m_d3dContext->m_ambientColor.x);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##6")) {
        m_d3dContext->m_ambientColor = { 1,1,1,1 };
    }

    ImGui::Text(u8"환경광(ambient) : 강도");
    ImGui::SliderFloat("##AmbientStrength", &m_d3dContext->m_ambientStrength,0.0f,1.0f);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##7")) {
        m_d3dContext->m_ambientStrength = 0.1f;
    }

    ImGui::Text(u8"확산광(diffuse) : 강도");
    ImGui::SliderFloat("##DiffuseStrength", &m_d3dContext->m_diffuseStrength, 0.0f, 1.0f);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##8")) {
        m_d3dContext->m_diffuseStrength = 1.0f;
    }

    ImGui::Text(u8"정반사광(specular) : 강도");
    ImGui::SliderFloat("##SpecularStrength", &m_d3dContext->m_specularStrength, 0.0f, 1.0f);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##9")) {
        m_d3dContext->m_specularStrength = 1.0f;
    }

    ImGui::Text(u8"광택지수(shininess)");
    static int shininessLevel = 12; //1~12
    ImGui::SliderInt("##shininess", &shininessLevel, 1, 12,"");
    m_d3dContext->m_shininess = static_cast<UINT>(pow(2, shininessLevel));
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##10")) {
        shininessLevel = 8;
        m_d3dContext->m_shininess = static_cast<UINT>(pow(2, shininessLevel));
    }

    ImGui::Text(u8"환경반사 강도 (cubemap reflection)");
    ImGui::SliderFloat("##reflectionFactor", &m_d3dContext->m_reflectionFactor, 0.0f, 1.0f);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기값##11")) {
        m_d3dContext->m_reflectionFactor = 0.6f;
    }

    ImGui::Checkbox(u8"광원설정 - 포인트 라이트", &m_d3dContext->m_isPointLight);

    std::string toStringStext = std::to_string(m_d3dContext->m_shininess);
    const char* stext = toStringStext.c_str();
    auto textSize = ImGui::CalcTextSize(stext);


    // 별도의 숫자 표시 (항상 맨 마지막에)
    ImVec2 pos = ImVec2((220 - textSize.x) * 0.5f - 32, 426); // 윈도우 안에서의 좌표
    ImGui::SetCursorPos(pos);
    ImGui::Text("%d", m_d3dContext->m_shininess);
    

    ImGui::End();
}

void MyEngine::MyImGui::Render()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void MyEngine::MyImGui::Uninitialize()
{
    if (m_isD3D11BackendInit)
        ImGui_ImplDX11_Shutdown();
    if (m_isWin32BackendInit)
        ImGui_ImplWin32_Shutdown();
    if (m_isImGuiInit)
        ImGui::DestroyContext();

    m_isImGuiInit = false;
    m_isWin32BackendInit = false;
    m_isD3D11BackendInit = false;
}

#endif //_DEBUG