//#ifdef _DEBUG
#include "MyImGui.h"
#include "MyD3DContext.h"

#include "TimeManager.h"

#include <string>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include "StaticMeshRenderer.h"
#include "Time.h"


void MyEngine::MyImGui::UpdateInfiniteDrag()
{
    ImGuiIO& io = ImGui::GetIO();

    static ImVec2 lastMousePosBeforeWarp = ImVec2(-1, 1);

    // 드래그 중이 아니면 리셋
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        lastMousePosBeforeWarp = ImVec2(-1, -1);
        return;
    }

    // 현재 윈도우 핸들 가져오기
    HWND hwnd = GetActiveWindow(); // 또는 메인 윈도우 핸들 저장해두기
    if (!hwnd) return;

    // 현재 마우스 위치 (스크린 좌표)
    POINT screenPos;
    GetCursorPos(&screenPos);

    // 윈도우 클라이언트 영역 정보
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    POINT clientOrigin = { 0, 0 };
    ClientToScreen(hwnd, &clientOrigin);

    int clientW = clientRect.right - clientRect.left;
    int clientH = clientRect.bottom - clientRect.top;

    // 클라이언트 영역 기준으로 변환
    int localX = screenPos.x - clientOrigin.x;
    int localY = screenPos.y - clientOrigin.y;

    // 워프 직전 델타 저장
    ImVec2 currentDelta = io.MouseDelta;

    // 경계 체크 및 워프 (여유 공간 10px)
    const int margin = 10;
    bool wrapped = false;
    POINT newScreenPos = screenPos;

    if (localX <= margin)
    {
        newScreenPos.x = clientOrigin.x + clientW - margin - 1;
        wrapped = true;
    }
    else if (localX >= clientW - margin)
    {
        newScreenPos.x = clientOrigin.x + margin + 1;
        wrapped = true;
    }

    if (localY <= margin)
    {
        newScreenPos.y = clientOrigin.y + clientH - margin - 1;
        wrapped = true;
    }
    else if (localY >= clientH - margin)
    {
        newScreenPos.y = clientOrigin.y + margin + 1;
        wrapped = true;
    }

    if (wrapped)
    {
        // 워프 직전 위치 저장
        lastMousePosBeforeWarp = io.MousePos;

        // 커서 워프
        SetCursorPos(newScreenPos.x, newScreenPos.y);

        // ImGui에게 새 위치 알리기 (클라이언트 좌표로)
        io.MousePos = ImVec2(
            (float)(newScreenPos.x - clientOrigin.x),
            (float)(newScreenPos.y - clientOrigin.y)
        );

        // 중요: Delta는 유지! 워프는 화면상 위치만 바꿀 뿐
        // 실제 마우스 이동량은 그대로 전달되어야 함
        io.MouseDelta = currentDelta;

        // WantSetMousePos로 ImGui에게 위치 변경 알림
        io.WantSetMousePos = true;
    }
    else if (lastMousePosBeforeWarp.x >= 0)
    {
        // 워프 직후 첫 프레임
        // 이때 Delta가 비정상적으로 클 수 있으므로 보정
        ImVec2 expectedDelta = ImVec2(
            io.MousePos.x - lastMousePosBeforeWarp.x,
            io.MousePos.y - lastMousePosBeforeWarp.y
        );

        // Delta가 비정상적으로 크면 (화면 반대편으로 점프) 무시
        float deltaLen = sqrtf(expectedDelta.x * expectedDelta.x +
            expectedDelta.y * expectedDelta.y);
        if (deltaLen > 100.0f) // threshold
        {
            io.MouseDelta = ImVec2(0, 0);
        }

        lastMousePosBeforeWarp = ImVec2(-1, -1);
    }
}

bool MyEngine::MyImGui::Initialize(MyD3DContext* myContext)
{
    m_d3dContext = myContext;

    this->m_hWnd = m_d3dContext->m_hWnd;
    this->m_pDevice = m_d3dContext->m_pd3dDevice.Get();
    this->m_pImmediateContext = m_d3dContext->m_pContext.Get();

    // 코어 컨텍스트 생성
    if (!ImGui::CreateContext()) {
        return false;
    }
    m_isImGuiInit = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 내비게이션 활성화
    m_defaultFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesKorean()); // 한글 폰트 설정
    m_bigFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 36.0f, NULL, io.Fonts->GetGlyphRangesKorean());

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
    ImGui::Text("  X : %.3g", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().x);
    ImGui::Text("  Y : %.3g", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().y);
    ImGui::Text("  Z : %.3g", m_d3dContext->m_pCamera->GetTransform()->GetLocalPosition().z);

    Vector3 euler = m_d3dContext->m_pCamera->GetTransform()->GetLocalRotation().ToEuler();

    ImGui::Text(u8"회전");
    ImGui::Text("  X : %.3g", XMConvertToDegrees(euler.x));
    ImGui::Text("  Y : %.3g", XMConvertToDegrees(euler.y));
    ImGui::Text("  z : %.2f", XMConvertToDegrees(euler.z));

    ImGui::Separator();

    constexpr float defFov = 75.0f;
    static float fov = defFov;
    ImGui::Text(u8"시야각");
    ImGui::SliderFloat("##FOV", &fov, 60.0f, 120.0f, "%g");
    m_d3dContext->m_pCamera->SetFOV(fov);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기화")) {
        fov = defFov;
    }

    constexpr float defNearPlane = 0.3f;
    static float nearPlane = 0.3f;
    ImGui::Text("Near Plane");
    ImGui::DragFloat("##Near", &nearPlane, 0.01f, 0.01f, 500.0f, "%g");

    m_d3dContext->m_pCamera->SetNearPlane(nearPlane);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기화##2")) {
        nearPlane = defNearPlane;
    }

    constexpr float defFarPlane = 1000.0f;
    static float farPlane = 1000.0f;
    ImGui::Text("Far Plane");
    ImGui::DragFloat("##Far", &farPlane, 0.01f, nearPlane + 0.01f, 1000.0f, "%g");
    m_d3dContext->m_pCamera->SetFarPlane(farPlane);
    ImGui::SameLine();
    if (ImGui::Button(u8"초기화##3")) {
        farPlane = defFarPlane;
    }

    ImGui::End();

	static bool showWindow = true;
    if (showWindow)
    {
        ImGui::PushFont(m_bigFont);
        ImGui::Begin("Read Me!!", &showWindow);
        ImGui::Text(u8"모니터 출력이 HDR을 지원하고 \nOS 설정에서 HDR모드를 켠상태인 경우 \nHDR모드가 자동으로 켜집니다 (백버퍼 설정 변경됨)");
        ImGui::End();
        ImGui::PopFont();
    }



    ImGui::SetNextWindowPos(ImVec2(5, 390), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(260, 374), ImGuiCond_Once);

    ImGui::Begin(u8"오브젝트 상태", nullptr, ImGuiWindowFlags_NoResize);

    static int objIdx = 0;
    static bool objIdxChanged = false;

    static size_t objCount = m_d3dContext->m_sceneObjects.size();

    if (!m_d3dContext->m_sceneObjects.empty())
    {
        objCount = m_d3dContext->m_sceneObjects.size();
        if (objIdx >= objCount)
        {
            objIdx = objCount - 1;
            objIdxChanged = true;
        }

        ImGui::Text(u8"오브젝트 인덱스");
        if (ImGui::SliderInt(u8"##오브젝트 인덱스", &objIdx, 0, m_d3dContext->m_sceneObjects.size() - 1))
        {
            objIdxChanged = true;
        }


        auto obj = m_d3dContext->m_sceneObjects[objIdx].get();
        auto obj_pos = obj->GetLocalPosition();

        if (objIdxChanged)
        {
            obj_pos = obj->GetLocalPosition();
        }

        ImGui::Text(u8"오브젝트 월드 위치");
        if (ImGui::DragFloat3("##obj_pos", &obj_pos.x, 0.05f))
        {
            obj->SetLocalPosition(obj_pos);
        }
        if (ImGui::IsItemActive())
        {
            UpdateInfiniteDrag();
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            obj->SetLocalPosition(obj_pos);
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값")) {
            obj_pos = Vector3::Zero;
            obj->SetLocalPosition(obj_pos);
        }


        ImGui::Text(u8"오브젝트 회전 값 (오일러)");
        static Vector3 obj_rot = obj->GetLocalEulerRotation();

        if (objIdxChanged)
        {
            obj_rot = obj->GetLocalEulerRotation();
        }

        if (ImGui::DragFloat3("##obj1_rot", &obj_rot.x, 0.1f))
        {
            obj->SetLocalEulerRotation(obj_rot);
        }
        if (ImGui::IsItemActive())
        {
            UpdateInfiniteDrag();
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            obj->SetLocalEulerRotation(obj_rot);
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##2")) {
            obj_rot = Vector3::Zero;
            obj->SetLocalEulerRotation(obj_rot);
        }


        ImGui::Text(u8"오브젝트 스케일 값");
        static Vector3 obj_scale = obj->GetLocalScale();

        if (objIdxChanged)
        {
            obj_scale = obj->GetLocalScale();
            objIdxChanged = false;
        }

        if (ImGui::DragFloat3("##obj1_scale", &obj_scale.x, 0.001f))
        {
            obj->SetLocalScale(obj_scale);
        }
        if (ImGui::IsItemActive())
        {
            UpdateInfiniteDrag();
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            obj->SetLocalScale(obj_scale);
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##14")) {
            obj_scale = Vector3::One;
            obj->SetLocalScale(obj_scale);
        }

        ImGui::Separator();

        ImGui::Text(u8"빛");

        ImGui::ColorEdit3("##Light1Color", &m_d3dContext->m_lightColor.x);
        if (ImGui::IsItemActive())
        {
            UpdateInfiniteDrag();
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##3")) {
            m_d3dContext->m_lightColor = { 1, 0.8823529411764706f,0.8352941176470588f,1 };;
        }

        constexpr Vector3 light_defEulerRot = { -42.8f,74.5f,0 };
        static Vector3 light_rot = m_d3dContext->m_pDirectionalLightT->GetLocalEulerRotation();

        if (ImGui::DragFloat3("##LightRot", &light_rot.x, 0.1f))
        {
            m_d3dContext->m_pDirectionalLightT->SetLocalEulerRotation(light_rot);
        }
        if (ImGui::IsItemActive())
        {
            UpdateInfiniteDrag();
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            m_d3dContext->m_pDirectionalLightT->SetLocalEulerRotation(light_rot);
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##4")) {
            light_rot = light_defEulerRot;
            m_d3dContext->m_pDirectionalLightT->SetLocalEulerRotation(light_defEulerRot);
        }

        //ImGui::Text(u8"환경광(ambient) : 색");
        //ImGui::ColorEdit3("##AmbientColor", &m_d3dContext->m_ambientColor.x);
        //if (ImGui::IsItemActive())
        //{
        //    UpdateInfiniteDrag();
        //}
        //ImGui::SameLine();
        //if (ImGui::Button(u8"초기값##6")) {
        //    m_d3dContext->m_ambientColor = { 0.9255f,0.5059f,0.7490f,1 };
        //}

        ImGui::Text(u8"거칠기(Roughness) : 강도");
        ImGui::SliderFloat("##AmbientStrength", &m_d3dContext->m_ambientStrength, 0.0f, 1.0f);
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##7")) {
            m_d3dContext->m_ambientStrength = 0.4f;
        }

        ImGui::Text(u8"금속성(metallic) : 강도");
        ImGui::SliderFloat("##DiffuseStrength", &m_d3dContext->m_diffuseStrength, 0.0f, 1.0f);
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##8")) {
            m_d3dContext->m_diffuseStrength = 1.0f;
        }

        ImGui::Text(u8"빛 HDR(Light HDR) : 강도");
        ImGui::SliderFloat("##SpecularStrength", &m_d3dContext->m_specularStrength, 1.0f, 30.0f);
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##9")) {
            m_d3dContext->m_specularStrength = 15.0f;
        }

        ImGui::Text(u8"환경광 (ambient) : 강도");
        ImGui::SliderFloat("##RimLightStrength", &m_d3dContext->m_rimLightStrength, 0.0f, 1.0f);
        ImGui::SameLine();
        if (ImGui::Button(u8"초기값##10")) {
            m_d3dContext->m_rimLightStrength = 1.0f;
        }

        //ImGui::Text(u8"확산광 그라디언트(diffuse gradient) : 강도");
        //ImGui::SliderFloat("##DiffuseGradientStrength", &m_d3dContext->m_diffuseGradientStrength, 0.0f, 1.0f);
        //ImGui::SameLine();
        //if (ImGui::Button(u8"초기값##그라디언트")) {
        //    m_d3dContext->m_diffuseGradientStrength = 0.3125f;
        //}

        //ImGui::Text(u8"정반사광(specular) : 강도");
        //ImGui::SliderFloat("##SpecularStrength", &m_d3dContext->m_specularStrength, 0.0f, 1.0f);
        //ImGui::SameLine();
        //if (ImGui::Button(u8"초기값##9")) {
        //    m_d3dContext->m_specularStrength = 0.228f;
        //}

        //ImGui::Text(u8"역광(rim light) : 강도");
        //ImGui::SliderFloat("##RimLightStrength", &m_d3dContext->m_rimLightStrength, 0.0f, 1.0f);
        //ImGui::SameLine();
        //if (ImGui::Button(u8"초기값##16")) {
        //    m_d3dContext->m_rimLightStrength = 1.0f;
        //}

        //ImGui::Text(u8"광택지수(shininess)");
        //static int shininessLevel = 9; //1~12
        //ImVec2 shininessUIPos = ImGui::GetCursorPos();
        //ImGui::SliderInt("##shininess", &shininessLevel, 1, 12, "");
        //m_d3dContext->m_shininess = pow(2, shininessLevel);
        //ImGui::SameLine();
        //if (ImGui::Button(u8"초기값##10")) {
        //    shininessLevel = 9;
        //    m_d3dContext->m_shininess = pow(2, shininessLevel);
        //}

        //ImGui::Text(u8"환경반사 강도 (cubemap reflection)");
        //ImGui::SliderFloat("##reflectionFactor", &m_d3dContext->m_reflectionFactor, 0.0f, 1.0f);
        //ImGui::SameLine();
        //if (ImGui::Button(u8"초기값##11")) {
        //    m_d3dContext->m_reflectionFactor = 0.005f;
        //}

        //std::string toStringStext = std::to_string(m_d3dContext->m_shininess);
        //const char* stext = toStringStext.c_str();
        //auto textSize = ImGui::CalcTextSize(stext);

        //// 별도의 숫자 표시 (항상 맨 마지막에)
        //ImVec2 pos = ImVec2((260 - textSize.x) * 0.5f - 39, shininessUIPos.y + 3); // 윈도우 안에서의 좌표
        //ImGui::SetCursorPos(pos);
        //ImGui::Text("%d", m_d3dContext->m_shininess);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(1600 - 225, 5), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(220, 86), ImGuiCond_Once);

    //ImGui::Begin(u8"렌더러 상태");
    //ImGui::Checkbox(u8"메쉬 넘버로 그리기", &DebugStatusUI::MeshRenderer::limitDrawOption);
    //ImGui::DragInt(u8"메쉬 넘버", &DebugStatusUI::MeshRenderer::meshNum);
    //if (ImGui::IsItemActive())
    //{
    //    UpdateInfiniteDrag();
    //}

    //ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(1600 - 225, 91 + 5), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(220, 130), ImGuiCond_Once);

    ImGui::Begin(u8"애니메이션 상태");

    if (!m_d3dContext->m_sceneObjects.empty())
    {
        auto animationMeshRenderer = dynamic_cast<SkinningMeshRenderer*>(m_d3dContext->m_meshRenderers[objIdx].get());
        if (animationMeshRenderer)
        {
            float animTime = static_cast<float>(animationMeshRenderer->GetTime());
            float duration = static_cast<float>(animationMeshRenderer->GetDuration());

            ImGui::Text(u8"애니메이션 시간");

            if (ImGui::SliderFloat(u8"##애니메이션 시간", &animTime, 0.0f, duration))
            {
                animationMeshRenderer->SetTime(animTime);
                animationMeshRenderer->Pause();
            }
            else
            {
                animationMeshRenderer->Play();
            }

            float animSpeed = static_cast<float>(animationMeshRenderer->GetSpeed());
            ImGui::Text(u8"애니메이션 속도");
            ImGui::DragFloat(u8"##애니메이션 속도", &animSpeed, 0.01f, 0.0f, 8.0f);
            animationMeshRenderer->SetSpeed(animSpeed);
            ImGui::SameLine();
            if (ImGui::Button(u8"초기값##12")) {
                animationMeshRenderer->SetSpeed(1.0f);
            }
        }
        else
        {
            ImGui::Text(u8"올바른 오브젝트를 선택해주세요!\n스키닝 메쉬렌더러가 아닙니다.\n\n왼쪽 패널에서 오브젝트 \n인덱스를 설정해주세요.");
        }
    }

    ImGui::End();

    //ImGui::SetNextWindowPos(ImVec2(1600 - 280, 480), ImGuiCond_Once);
    //ImGui::SetNextWindowSize(ImVec2(275, 350), ImGuiCond_Once);
  
    //ImGui::Begin(u8"그림자 맵 디버그");

    //// 그림자 맵 SRV를 ImGui로 표시
    //if (m_d3dContext->m_pShadowSRV)
    //{
    //    // 크기 지정 (픽셀 단위)
    //    ImVec2 imageSize(256, 256);  // 원하는 크기로 조정

    //    // ImGui::Image는 void* 타입을 받지만, SRV 포인터를 그대로 캐스팅
    //    ImGui::Image(
    //        (ImTextureID)m_d3dContext->m_pShadowSRV.Get(),  // SRV 포인터
    //        ImVec2(256, 256)  // 이미지 크기
    //    );

    //    // 호버 시 확대 표시 (선택사항)
    //    if (ImGui::IsItemHovered())
    //    {
    //        ImGui::BeginTooltip();
    //        ImGui::Image(
    //            (ImTextureID)m_d3dContext->m_pShadowSRV.Get(),  // SRV 포인터
    //            ImVec2(512, 512)  // 이미지 크기
    //        );
    //        ImGui::EndTooltip();
    //    }

    //    ImGui::DragFloat(u8"프로젝션 Near", &m_d3dContext->m_lightProjectNear, 0.01f, 0.01f, 50.0f);
    //    ImGui::DragFloat(u8"프로젝션 Far", &m_d3dContext->m_lightProjectFar, 1.0f, 50.0f, 1500.0f);
    //}
    //else
    //{
    //    ImGui::Text(u8"Shadow Map이 초기화되지 않음");
    //}

    //ImGui::End();

    //ImGui::SetNextWindowPos(ImVec2(1600 - 230, 395), ImGuiCond_Once);
    //ImGui::SetNextWindowSize(ImVec2(225, 80), ImGuiCond_Once);

    //ImGui::Begin(u8"아웃라인 디버그");

    //ImGui::Text(u8"아웃라인 두께");
    //ImGui::DragFloat(u8"##아웃라인 두께", &m_d3dContext->m_outlineThickness, 0.001f);
    //if (ImGui::IsItemActive())
    //{
    //    UpdateInfiniteDrag();
    //}
    //ImGui::SameLine();
    //if (ImGui::Button(u8"초기값")) {
    //    m_d3dContext->m_outlineThickness = 0.02f;
    //}

    //ImGui::End();


    //ImGui::SetNextWindowPos(ImVec2(1600 - 230, 325), ImGuiCond_Once);
    //ImGui::SetNextWindowSize(ImVec2(225, 65), ImGuiCond_Once);

    //ImGui::Begin(u8"그라디언트 디버그");

    //ImGui::SliderFloat(u8"그라디언트 강도", &m_d3dContext->m_gradientIntensity, 0.0f, 1.0f);

    //ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(1600 - 225, 230), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(220, 90), ImGuiCond_Once);

    ImGui::Begin(u8"디버그 드로잉");

    ImGui::Checkbox(u8"활성화", &m_d3dContext->m_enableDebugDraw);
    ImGui::Checkbox(u8"zbuffer 사용", &m_d3dContext->m_enableDebugDrawZbuffer);

    ImGui::End();

    ImGui::Begin(u8"HDR - 색상 공간 설정");


    if (m_d3dContext->m_supportHDR)
        ImGui::Text(u8"HDR 지원함 -> PQ 공식 적용");
    else
        ImGui::Text(u8"HDR 지원하지 않음 -> ACES 필름효과 적용");

    ImGui::DragFloat(u8"노출 값", &m_d3dContext->m_exposure, 0.001f);

    ImGui::End();

    //ImGui::Begin(u8"메모리 디버그");    

    //ImGui::Text(u8"F키로 카메라 앞에 스키닝 메쉬 생성, G키로 메쉬 순차 삭제");

    //if (ImGui::Button("D3D-Trim()"))
    //{
    //    m_d3dContext->m_dxgiDevice->Trim();
    //}

    //static DebugStatusUI::DRamDebugData data1;
    //static DebugStatusUI::GPURamDebugData data2;

    //auto toMB = [](UINT64 v) { return v / (1024.0 * 1024.0); };

    //static float updateInterval = 0.0f;
    //updateInterval += TIME_GET_DELTA();

    //if (updateInterval > 0.03125f)
    //{
    //    updateInterval = 0.0f;

    //    data1 = DebugStatusUI::GetCpuMemoryUsage();
    //    data2 = DebugStatusUI::QueryGpuMemory(m_d3dContext->m_pd3dDevice);
    //}

    //if (data1.isValidData)
    //{
    //    auto ramUsageMB = toMB(data1.workingSet);
    //    auto ramPageMB = toMB(data1.PagefileUsage - data1.workingSet);
    //    ImGui::Text(u8"DRAM 사용량 %.1f MB",(float)ramUsageMB);
    //    ImGui::Text(u8"메모리 페이지 %.1f MB", (float)ramPageMB);
    //}
    //if (data2.isValidData)
    //{
    //    ImGui::Text(u8"비디오 메모리 사용량 %.1f MB", (float)data2.usage);
    //}
    //
    //ImGui::End();
    

    if(m_d3dContext->m_enableDebugDraw)
    for (size_t i = 0; i < m_d3dContext->m_sceneObjects.size(); i++)
    {
        auto& pTransform = m_d3dContext->m_sceneObjects[i];
        auto& cam = m_d3dContext->m_pCamera;
        auto pos = pTransform->GetWorldPosition();

        // 1. Vector4로 확장
        DirectX::SimpleMath::Vector4 clipPos(pos.x, pos.y, pos.z, 1.0f);

        // 2. ViewProjection 변환 적용
        clipPos = Vector4::Transform(clipPos, cam->GetViewMatrix() * cam->GetProjMatrix());

        if (clipPos.w < 0.0f)
            continue;

        // 3. 원근 나누기
        clipPos /= clipPos.w;

        // 4. 결과 (NDC space)
        DirectX::SimpleMath::Vector3 ndcPos(clipPos.x, clipPos.y, clipPos.z);


        float screenX = (ndcPos.x * 0.5f + 0.5f) * m_d3dContext->m_width;
        float screenY = (-ndcPos.y * 0.5f + 0.5f) * m_d3dContext->m_height; // y 뒤집기 주의!

        std::string text = "obj" + std::to_string(i);

        ImVec2 screenPos(screenX, screenY);
        ImGui::GetBackgroundDrawList()->AddText(screenPos, IM_COL32(255, 255, 255, 255), text.c_str());
    }
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

//#endif //_DEBUG

DebugStatusUI::DRamDebugData DebugStatusUI::GetCpuMemoryUsage()
{
    DRamDebugData data;
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        data.isValidData = true;
        data.workingSet = pmc.WorkingSetSize;        // 실제 물리 메모리(RSS)
        data.privateBytes = pmc.PrivateUsage;       // Private bytes (Windows 8.1+)
        data.PagefileUsage = pmc.PagefileUsage;
        //std::cout << "RSS (bytes): " << workingSet << ", PrivateBytes: " << privateBytes << "\n";
        // 페이지 수로 변환
        data.si; GetSystemInfo(&data.si);
        data.pageSize = data.si.dwPageSize;
        //std::cout << "RSS pages: " << (workingSet + pageSize - 1) / pageSize << "\n";
    }

    return data;
}

DebugStatusUI::GPURamDebugData DebugStatusUI::QueryGpuMemory(ComPtr<ID3D11Device> d3dDevice)
{
    GPURamDebugData data;
    ComPtr<IDXGIDevice> dxgiDevice;
    if (SUCCEEDED(d3dDevice.As(&dxgiDevice)))
    {
        ComPtr<IDXGIAdapter> adapter;
        if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter)))
        {
            // Try to get IDXGIAdapter3
            ComPtr<IDXGIAdapter3> adapter3;
            if (SUCCEEDED(adapter.As(&adapter3)))
            {
                data.isValidData = true;
                DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
                adapter3->QueryVideoMemoryInfo(
                    0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);

                auto toMB = [](UINT64 v) { return v / (1024.0 * 1024.0); };
                data.usage = toMB(info.CurrentUsage);
                data.page = toMB(info.CurrentReservation);
            }
        }
    }

    return data;
}