#include "Time.h"
#include "MyD3DContext.h"
#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

bool MyEngine::MyD3DContext::Initialize(HWND hWnd, int width, int height)
{
    m_hWnd = hWnd;
    m_width = width;
    m_height = height;

    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    // 디버그용 디바이스 플래그 설정
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //_DEBUG

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, m_pd3dDevice.GetAddressOf(), &m_featureLevel, m_pContext.GetAddressOf());

        if (hr == E_INVALIDARG)
        {
            // DirectX 11.0 플랫폼은 D3D_FEATURE_LEVEL_11_1를 인식하지 못하기 때문에 없이 한번 더 시도
            hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                D3D11_SDK_VERSION, m_pd3dDevice.GetAddressOf(), &m_featureLevel, m_pContext.GetAddressOf());
        }

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return false;

    // DXGI 팩토리를 디바이스에서 부터 얻기
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = m_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return false;

    // 스왑체인 생성
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2)
    {
        // DirectX 11.1 이거나 이후 버전인 경우
        hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(m_pd3dDevice1.GetAddressOf()));
        if (SUCCEEDED(hr))
        {
            (void)m_pContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(m_pContext.GetAddressOf()));
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd(m_pd3dDevice.Get(), m_hWnd, &sd, nullptr, nullptr, m_pSwapChain1.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            hr = m_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(m_pSwapChain.GetAddressOf()));
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 시스템인 경우
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = m_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(m_pd3dDevice.Get(), &sd, m_pSwapChain.GetAddressOf());
    }

    // 이 튜토리얼 코드는 풀스크린 스왑체인을 관리하지 않음, 따라서 ALT+ENTER 단축키를 제외시킴
    dxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
        return false;

    // 렌더 타겟 뷰 생성
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return false;

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, m_pRenderTargetView.GetAddressOf());
    pBackBuffer->Release();
    if (FAILED(hr))
        return false;

    // 뎁스 스텐실 텍스쳐 생성
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, NULL, m_pDepthStencil.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 뎁스 스텐실 뷰 생성
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf());
    if (FAILED(hr))
        return false;


    m_pContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

    // 뷰포트 설정
    m_vp.Width = (FLOAT)width;
    m_vp.Height = (FLOAT)height;
    m_vp.MinDepth = 0.0f;
    m_vp.MaxDepth = 1.0f;
    m_vp.TopLeftX = 0;
    m_vp.TopLeftY = 0;
    m_pContext->RSSetViewports(1, &m_vp);

    // 래스터라이저 상태 생성 및 설정
    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_BACK; //양면 드로우 허용
    rastDesc.FrontCounterClockwise = TRUE;  // RH 좌표계용으로 변경
    rastDesc.DepthBias = 0;
    rastDesc.DepthBiasClamp = 0.0f;
    rastDesc.SlopeScaledDepthBias = 0.0f;
    rastDesc.DepthClipEnable = TRUE;
    rastDesc.ScissorEnable = FALSE;
    rastDesc.MultisampleEnable = FALSE;
    rastDesc.AntialiasedLineEnable = FALSE;

    hr = m_pd3dDevice->CreateRasterizerState(&rastDesc, m_pDefRasterizerState.GetAddressOf());
    if (FAILED(hr))
        return false;

    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FrontCounterClockwise = FALSE;  // 스카이박스용
    rastDesc.DepthBias = 0;
    rastDesc.DepthBiasClamp = 0.0f;
    rastDesc.SlopeScaledDepthBias = 0.0f;
    rastDesc.DepthClipEnable = TRUE;
    rastDesc.ScissorEnable = FALSE;
    rastDesc.MultisampleEnable = FALSE;
    rastDesc.AntialiasedLineEnable = FALSE;

    hr = m_pd3dDevice->CreateRasterizerState(&rastDesc, m_pClockWiseRasterizerState.GetAddressOf());
    if (FAILED(hr))
        return false;

    //래스터라이저 상태 설정
    m_pContext->RSSetState(m_pDefRasterizerState.Get());

    // 샘플러 상태 생성
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerLinear.GetAddressOf());
    if (FAILED(hr))
        return false;

    //알파 블렌드 상태 설정
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    m_pd3dDevice->CreateBlendState(&blendDesc, &m_pBlendState);

    //뎁스 스텐실 상태 설정
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;                     // 깊이 테스트 활성화
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 버퍼 쓰기 활성화
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;      // 깊이가 작을수록 앞에 있음
    depthStencilDesc.StencilEnable = FALSE;                  // 스텐실은 비활성화

    m_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_pOpaqueState);

    depthStencilDesc = D3D11_DEPTH_STENCIL_DESC{};
    depthStencilDesc.DepthEnable = TRUE;                     // 깊이 테스트 활성화
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 버퍼 쓰기 비활성
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;      // 깊이가 작을수록 앞에 있음
    depthStencilDesc.StencilEnable = FALSE;                  // 스텐실은 비활성화

    m_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_pTransparentState);

    Material::InitDefaultShaders(m_pd3dDevice.Get());

//#ifdef _DEBUG
    // ImGui 초기화
    if (!m_imgui.Initialize(this))
        return false;
//#endif //_DEBUG

    return true;
}

bool MyEngine::MyD3DContext::InitializeScene()
{
    AssimpConverter::Initialize(m_pContext.Get());

    HRESULT hr = S_OK;

    InitCube();
    InitSkyBox();
    InitShadowMapTex();

    //상수 버퍼 생성
    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory(&cbDesc, sizeof(cbDesc));
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(MyConstantBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;

    hr = m_pd3dDevice->CreateBuffer(&cbDesc, nullptr, m_pConstantBuffer.GetAddressOf());
    if (FAILED(hr))
        return false;

    //카메라 생성
    m_pCamera = std::make_unique<Camera>();
    m_pCamera->GetTransform()->SetWorldPosition(-5.0f, 4.8f, 10.9f);
    m_pCamera->GetTransform()->SetWorldEulerRotation(-17.0f, -20.0f, 0.0f);
    m_pCamera->SetAspectRatio((float)m_width, (float)m_height);

    m_pDirectionalLightT = std::make_unique<Transform>();
    m_pDirectionalLightT->SetLocalEulerRotation({ -90,0,0 });
    m_pDirectionalLightT->SetLocalScale({ 1,1,1 });

    //오브젝트 생성
    m_sceneObjects.push_back(std::make_unique<Transform>());
    m_sceneObjects.push_back(std::make_unique<Transform>());
    m_sceneObjects.push_back(std::make_unique<Transform>());
    m_sceneObjects.push_back(std::make_unique<Transform>());

    auto obj1 = m_sceneObjects[0].get();
    obj1->SetWorldPosition(0, 0, -45.0f);
    obj1->SetLocalScale(0.072f, 0.072f, 0.072f);

    auto obj2 = m_sceneObjects[1].get();
    obj2->SetWorldPosition(0.0f, 0.0f, 1.5f);
    obj2->SetLocalEulerRotation(0.0, 0.0f, 0.0f);
    obj2->SetLocalScale(5, 5, 5);

    auto obj3 = m_sceneObjects[2].get();
    obj3->SetWorldPosition(0, 0, 0);
    obj3->SetLocalScale(0.05f, 0.05f, 0.05f);

    auto obj4 = m_sceneObjects[3].get();
    obj4->SetWorldPosition(0, 0, 0);
    obj4->SetLocalScale(0.05f, 0.05f, 0.05f);

    AssimpConverter::SetLoadMaterialType(AssimpConverter::LoadMaterialType::BlinnPhongToon);
    m_pSkinningMeshRenderers.push_back(AssimpConverter::LoadSkinningMeshRendererFromFile("Resources/Models/SkinningTest.fbx"));
    m_pStaticMeshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/Miyu_Akey_Rigging.obj"));
    AssimpConverter::SetLoadMaterialType(AssimpConverter::LoadMaterialType::BlinnPhong);
    m_pStaticMeshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/Ground.fbx"));
    AssimpConverter::SetLoadMaterialType(AssimpConverter::LoadMaterialType::BlinnPhongToon);
    m_pStaticMeshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/zeldaPosed001.fbx"));
    //m_pRigidMeshRenderers.push_back(AssimpConverter::LoadRigidMeshRendererFromFile("Resources/Models/BoxHuman.fbx"));

    return true;
}

bool MyEngine::MyD3DContext::InitCube()
{
    HRESULT hr = S_OK;

    //컴파일 정보 저장용 객체
    ID3DBlob* pVSBlob = nullptr;

    // === 기본 셰이더 로드 ===
    hr = CompileShaderFromFile(L"Resources/Shaders/testVS.hlsl", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"정점 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf());
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return false;
    }

    //인풋 레이아웃 (셰이더 코드 바인딩) 설정
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0,D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    //인풋 레이아웃 생성
    hr = m_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), m_pCubeInputLayout.GetAddressOf());
    pVSBlob->Release();
    if (FAILED(hr))
        return false;

    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"Resources/Shaders/testPS.hlsl", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf());
    pPSBlob->Release();
    if (FAILED(hr))
        return false;

    hr = CompileShaderFromFile(L"Resources/Shaders/testPS.hlsl", "PSSolid", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"단일 픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, m_pPixelShaderSolid.GetAddressOf());
    pPSBlob->Release();
    if (FAILED(hr))
        return false;

    //정점 정의
	MyVertex vertices[] =
	{
		// Top face (Y = 1)
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },

		// Bottom face (Y = -1)
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },

		// Left face (X = -1)
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),    XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),   XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),    XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),     XMFLOAT2(0.0f, 0.0f) },

		// Right face (X = 1)
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f),   XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f),  XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f),   XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f),    XMFLOAT2(1.0f, 0.0f) },

		// Back face (Z = -1)
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),   XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),    XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),     XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),    XMFLOAT2(1.0f, 0.0f) },

		// Front face (Z = 1)
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),  XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),   XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),  XMFLOAT2(0.0f, 0.0f) },
	};

    //정점 버퍼 정의
    D3D11_BUFFER_DESC vbDesc = {};
    m_vertexCount = ARRAYSIZE(vertices);
    vbDesc.ByteWidth = sizeof(MyVertex) * m_vertexCount;
    vbDesc.CPUAccessFlags = 0;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.MiscFlags = 0;
    vbDesc.Usage = D3D11_USAGE_DEFAULT;

    //정점 버퍼 생성
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;	// 버퍼를 생성할때 복사할 데이터의 주소 설정 
    hr = m_pd3dDevice->CreateBuffer(&vbDesc, &vbData, m_pVertexBuffer.GetAddressOf());

    if (FAILED(hr))
        return false;

    m_vertexBufferStride = sizeof(MyVertex);
    m_vertexBufferOffset = 0;

    //인덱스 정의
    UINT indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    //인덱스 버퍼 정의
    D3D11_BUFFER_DESC ibDesc;
    m_indexCount = ARRAYSIZE(indices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(UINT) * m_indexCount;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;
    ibDesc.MiscFlags = 0;

    //인덱스 버퍼 생성
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = indices;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    hr = m_pd3dDevice->CreateBuffer(&ibDesc, &InitData, m_pIndexBuffer.GetAddressOf());
    if (FAILED(hr))
        return false;

    ScratchImage image;

    //텍스쳐 로드
    //hr = LoadFromDDSFile(L"Resources/Textures/seafloor.dds", DDS_FLAGS_NONE, nullptr, image);
    hr = LoadFromWICFile(L"Resources/Textures/Lut.png", WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(hr))
        return false;

    hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), m_pCubeTextureRV.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = LoadFromDDSFile(L"Resources/Textures/normal_mapping_normal_map.dds", DDS_FLAGS_NONE, nullptr, image);
    if (FAILED(hr))
        return false;
    hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), m_pCubeNormalMapRV.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = LoadFromDDSFile(L"Resources/Textures/spec_mapping.dds", DDS_FLAGS_NONE, nullptr, image);
    if (FAILED(hr))
        return false;
    hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), m_pCubeSpecularMapRV.GetAddressOf());
    if (FAILED(hr))
        return false;

    return true;
}
bool MyEngine::MyD3DContext::InitSkyBox()
{
    HRESULT hr = S_OK;
    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pPSBlob = nullptr;

    // === 스카이박스 셰이더 로드 ===
    hr = CompileShaderFromFile(L"Resources/Shaders/SkyBoxVS.hlsl", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"스카이박스 정점 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, m_pSkyBoxVShader.GetAddressOf());
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return false;
    }

    //인풋 레이아웃 (셰이더 코드 바인딩) 설정
    D3D11_INPUT_ELEMENT_DESC skyBoxlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(skyBoxlayout);

    //인풋 레이아웃 생성
    hr = m_pd3dDevice->CreateInputLayout(skyBoxlayout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), m_pSkyBoxInputLayout.GetAddressOf());
    pVSBlob->Release();
    if (FAILED(hr))
        return false;

    pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"Resources/Shaders/SkyBoxPS.hlsl", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"스카이박스 픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, m_pSkyBoxPShader.GetAddressOf());
    pPSBlob->Release();
    if (FAILED(hr))
        return false;



    //스카이박스 정점 정의
    SkyBoxVertex skyboxVertices[] =
    {
        // 상단 (+Y)
        { XMFLOAT3(-1.0f,  1.0f, -1.0f) }, // 0
        { XMFLOAT3(1.0f,  1.0f, -1.0f) }, // 1
        { XMFLOAT3(1.0f,  1.0f,  1.0f) }, // 2
        { XMFLOAT3(-1.0f,  1.0f,  1.0f) }, // 3

        // 하단 (-Y)
        { XMFLOAT3(-1.0f, -1.0f, -1.0f) }, // 4
        { XMFLOAT3(1.0f, -1.0f, -1.0f) }, // 5
        { XMFLOAT3(1.0f, -1.0f,  1.0f) }, // 6
        { XMFLOAT3(-1.0f, -1.0f,  1.0f) }, // 7
    };

    //스카이박스 정점 버퍼 정의
    D3D11_BUFFER_DESC vbDesc = {};
    m_skyBoxVertexCount = ARRAYSIZE(skyboxVertices);
    vbDesc.ByteWidth = sizeof(SkyBoxVertex) * m_skyBoxVertexCount;
    vbDesc.CPUAccessFlags = 0;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.MiscFlags = 0;
    vbDesc.Usage = D3D11_USAGE_DEFAULT;

    //스카이박스 정점 버퍼 생성
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = skyboxVertices;	// 버퍼를 생성할때 복사할 데이터의 주소 설정 
    hr = m_pd3dDevice->CreateBuffer(&vbDesc, &vbData, m_pSkyBoxVertexBuffer.GetAddressOf());

    if (FAILED(hr))
        return false;

    m_skyBoxVertexBufferStride = sizeof(SkyBoxVertex);
    m_skyBoxVertexBufferOffset = 0;

    //인덱스 정의
    UINT skyboxIndices[] =
    {
        // 상단 (+Y) - RH 기준 CCW
        0, 2, 1,
        0, 3, 2,

        // 하단 (-Y)
        4, 5, 6,
        4, 6, 7,

        // 왼쪽 (-X)
        4, 7, 3,
        4, 3, 0,

        // 오른쪽 (+X)
        1, 2, 6,
        1, 6, 5,

        // 앞면 (+Z)
        3, 6, 2,
        3, 7, 6,

        // 뒷면 (-Z)
        4, 0, 1,
        4, 1, 5
    };

    //스카이박스 인덱스 버퍼 정의
    D3D11_BUFFER_DESC ibDesc;
    m_skyBoxIndexCount = ARRAYSIZE(skyboxIndices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(UINT) * m_skyBoxIndexCount;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;
    ibDesc.MiscFlags = 0;

    //스카이박스 인덱스 버퍼 생성
    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = skyboxIndices;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    hr = m_pd3dDevice->CreateBuffer(&ibDesc, &InitData, m_pSkyBoxIndexBuffer.GetAddressOf());
    if (FAILED(hr))
        return false;

    //텍스쳐 로드

    ScratchImage image;

    DirectX::TexMetadata metadata;
    hr = LoadFromDDSFile(L"Resources/Textures/cubemap.dds", DDS_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
        return false;
    if (!metadata.IsCubemap())
    {
        MessageBox(nullptr,
            L"스카이박스 텍스쳐가 큐브맵이 아닙니다.", L"오류", MB_OK);
        return false;
    }

    hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), m_pSkyBoxTextureRV.GetAddressOf());
    if (FAILED(hr))
        return false;

    return true;
}

bool MyEngine::MyD3DContext::InitShadowMapTex()
{
    HRESULT hr = S_OK;

    // 깊이 텍스처 생성
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = SHADOW_MAP_SIZE;
    texDesc.Height = SHADOW_MAP_SIZE;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS;  // 깊이+SRV 겸용
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    hr = m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, m_pShadowTex.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 깊이 스텐실 뷰 (DSV)
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = m_pd3dDevice->CreateDepthStencilView(m_pShadowTex.Get(), &dsvDesc, m_pShadowDSV.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 리소스뷰 (RSV)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    // 수정: TYPELESS 리소스의 SRV 포맷은 R32_FLOAT을 사용합니다.
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    // m_pShadowSRV는 ID3D11ShaderResourceView* 멤버 변수여야 합니다.
    hr = m_pd3dDevice->CreateShaderResourceView(m_pShadowTex.Get(), &srvDesc, m_pShadowSRV.GetAddressOf());
    if (FAILED(hr))
        return false;

    //컴파일 정보 저장용 객체
    ID3DBlob* pVSBlob = nullptr;

    // === 정점 셰이더 로드 ===
    hr = CompileShaderFromFile(L"Resources/Shaders/ShadowMapVS.hlsl", "main", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"정점 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, m_pShadowMapVS.GetAddressOf());
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return false;
    }

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

    samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_pd3dDevice->CreateSamplerState(&samplerDesc, &m_pShadowSampler);
    if (FAILED(hr))
        return false;

    D3D11_RASTERIZER_DESC rd;
    ZeroMemory(&rd, sizeof(rd));
    rd.CullMode = D3D11_CULL_BACK;
    rd.FillMode = D3D11_FILL_SOLID;
    rd.FrontCounterClockwise = TRUE;
    rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
    rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
    rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rd.DepthClipEnable = TRUE;
    rd.ScissorEnable = FALSE;
    rd.MultisampleEnable = FALSE;
    rd.AntialiasedLineEnable = FALSE;
    hr = m_pd3dDevice->CreateRasterizerState(&rd, &m_pShadowMapRasterizerState);
    if (FAILED(hr))
        return false;


    m_shadowViewport = {};
    m_shadowViewport.TopLeftX = 0.0f;
    m_shadowViewport.TopLeftY = 0.0f;
    m_shadowViewport.Width = (FLOAT)SHADOW_MAP_SIZE;
    m_shadowViewport.Height = (FLOAT)SHADOW_MAP_SIZE;
    m_shadowViewport.MinDepth = 0.0f;
    m_shadowViewport.MaxDepth = 1.0f;

    return true;
}

void MyEngine::MyD3DContext::Clear()
{
    float ClearColor[4] = { 0.0f, 0.9f, 0.6f, 1.0f }; // RGBA

    m_pContext->ClearRenderTargetView(m_pRenderTargetView.Get(), ClearColor);
    m_pContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void MyEngine::MyD3DContext::Render()
{
    // 추후에 변경할 예정이지만 일단 씬 내용을 업데이트
    m_pCamera->InputUpdate(Time::instance->GetDeltaTime());

    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);

    MyConstantBuffer cb;
    cb.mWorld = XMMatrixIdentity();

    //  <=============== 첫번째 패스(그림자 맵)
    // 컬러 렌더타겟은 사용 안 함
    m_pContext->PSSetShader(nullptr, nullptr, 0);
    m_pContext->OMSetRenderTargets(0, nullptr, m_pShadowDSV.Get());
    // 깊이 초기화
    m_pContext->ClearDepthStencilView(m_pShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_pContext->OMSetDepthStencilState(m_pOpaqueState.Get(), 0);


    m_pContext->RSSetState(m_pShadowMapRasterizerState.Get()); //기본 래스터라이저 상태로 복귀

    // 뷰포트 세팅(텍스쳐 사이즈로)
    m_pContext->RSSetViewports(1, &m_shadowViewport);
    cb.vLightColor = m_lightColor;
    auto lightFwd = m_pDirectionalLightT->GetWorldMatrix().Forward();
    //m_pDirectionalLightT->SetLocalPosition(m_pCamera->GetTransform()->GetWorldPosition() + lightFwd * -SHADOW_MAP_DEPTH);
    m_pDirectionalLightT->SetLocalPosition(lightFwd * -SHADOW_MAP_DEPTH);
    auto xmLightDir = XMFLOAT4{ lightFwd.x,lightFwd.y,lightFwd.z,1 };
    cb.vLightDir = xmLightDir;

    // 뷰, 프로젝션 행렬 생성
    Matrix lightViewMat = m_pDirectionalLightT->GetWorldMatrix().Invert();
    Matrix lightProj = Matrix::CreateOrthographic(50.0f, 50.0f, m_lightProjectNear, m_lightProjectFar);

    // 최종 LightViewProjection 행렬
    Matrix lightViewProj = lightViewMat * lightProj;

    // 쉐이더 상수 버퍼에 세팅
    cb.mlightViewProj = lightViewProj.Transpose();
    cb.mView = lightViewMat.Transpose();
    cb.mProjection = lightProj.Transpose();

    m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

    auto& obj1 = m_sceneObjects[0];
    cb.mWorld = XMMatrixTranspose(obj1->GetWorldMatrix());
    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetInputLayout(m_pCubeInputLayout.Get());


    m_pContext->VSSetShader(Material::GetBlinnPhongVertexShader_SkinningBone(), nullptr, 0);
    m_pSkinningMeshRenderers[0]->Draw(m_pContext.Get(), true, false, false);

    m_pContext->VSSetShader(Material::GetBlinnPhongVertexShader(), nullptr, 0);

    auto& obj2 = m_sceneObjects[1];
    cb.mWorld = XMMatrixTranspose(obj2->GetWorldMatrix());
    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    m_pStaticMeshRenderers[0]->Draw(m_pContext.Get(), true, false, { 1,5 });

    auto& obj3 = m_sceneObjects[2];
    cb.mWorld = XMMatrixTranspose(obj3->GetWorldMatrix());
    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    m_pStaticMeshRenderers[1]->Draw(m_pContext.Get(), true, false);

    auto& obj4 = m_sceneObjects[3];
    cb.mWorld = XMMatrixTranspose(obj4->GetWorldMatrix());
    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    m_pStaticMeshRenderers[2]->Draw(m_pContext.Get(), true, false);

    //  <=============== 두번째 패스(장면)
    Clear();
    m_pContext->RSSetState(m_pDefRasterizerState.Get()); //기본 래스터라이저 상태로 복귀
    m_pContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());
    m_pContext->RSSetViewports(1, &m_vp);
    m_pContext->PSSetSamplers(1, 1, m_pShadowSampler.GetAddressOf());
    m_pContext->PSSetShaderResources(6, 1, m_pShadowSRV.GetAddressOf());
    m_pContext->PSSetShaderResources(5, 1, m_pCubeTextureRV.GetAddressOf());

    cb.mWorld = XMMatrixIdentity();
    cb.mView = XMMatrixTranspose(m_pCamera->GetViewMatrix());
    cb.mProjection = XMMatrixTranspose(m_pCamera->GetProjMatrix());
    cb.CameraPos = m_pCamera->GetTransform()->GetLocalPosition();
    cb.gradientIntensity = 1.0f;

    XMStoreFloat3(&cb.vLightPos, XMVectorScale(XMLoadFloat4(&xmLightDir), -1.25f));

    cb.vOutputColor = XMFLOAT4(0, 0, 0, 0);
    cb.ambientStr = m_ambientStrength;
    cb.diffuseStr = m_diffuseStrength;
    cb.specularStr = m_specularStrength;
    cb.shininess = m_shininess;
    cb.vAmbientColor = m_ambientColor;
    cb.reflectionFactor = m_reflectionFactor;


    //스카이박스 드로우
    m_pContext->PSSetShaderResources(1, 1, m_pSkyBoxTextureRV.GetAddressOf());
    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetInputLayout(m_pSkyBoxInputLayout.Get());
    m_pContext->IASetVertexBuffers(0, 1, m_pSkyBoxVertexBuffer.GetAddressOf(), &m_skyBoxVertexBufferStride, &m_skyBoxVertexBufferOffset);
    m_pContext->IASetIndexBuffer(m_pSkyBoxIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    m_pContext->PSSetShaderResources(0, 1, m_pSkyBoxTextureRV.GetAddressOf());
    m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //스카이박스는 시계방향으로 컬링
    m_pContext->VSSetShader(m_pSkyBoxVShader.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pSkyBoxPShader.Get(), nullptr, 0);
    m_pContext->DrawIndexed(m_skyBoxIndexCount, 0, 0);
    m_pContext->RSSetState(m_pDefRasterizerState.Get()); //기본 래스터라이저 상태로 복귀


    int modelIdx = 0;
    // <<======= 아웃라인 드로우
    for (auto& obj : m_sceneObjects)
    {
        m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //스카이박스는 시계방향으로 컬링

        cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
        m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pContext->IASetInputLayout(m_pCubeInputLayout.Get());

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());


        Material::BindOutlineShaders(m_pContext.Get());

        if (modelIdx == 0)
        {
            m_pContext->VSSetShader(Material::GetOutlineVertexShader_SkinningBone(), nullptr, 0);
            m_pSkinningMeshRenderers[0]->Draw(m_pContext.Get(), true, false, false);
        }
        //if (modelIdx == 1)
        //    m_pStaticMeshRenderers[0]->Draw(m_pContext.Get(), true,false);
        if (modelIdx == 3)
            m_pStaticMeshRenderers[2]->Draw(m_pContext.Get(), true, false);
        modelIdx++;

        m_pContext->RSSetState(m_pDefRasterizerState.Get()); //기본 래스터라이저 상태로 복귀
    }

    modelIdx = 0;
    // <<======= 씬 드로우
    for (auto& obj : m_sceneObjects)
    {
        cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
        m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pContext->IASetInputLayout(m_pCubeInputLayout.Get());

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());


        if (modelIdx == 0)
        {
            m_pSkinningMeshRenderers[0]->Draw(m_pContext.Get());
        }
        if (modelIdx == 1)
        {
            m_pStaticMeshRenderers[0]->Draw(m_pContext.Get());
        }
        if (modelIdx == 2)
        {
            cb.gradientIntensity = 0.0f;
            m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
            m_pStaticMeshRenderers[1]->Draw(m_pContext.Get());
        }
        if (modelIdx == 3)
        {
            m_pStaticMeshRenderers[2]->Draw(m_pContext.Get());
        }
        modelIdx++;

    }

    m_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_vertexBufferStride, &m_vertexBufferOffset);
    m_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

//#ifdef _DEBUG
    m_imgui.BeginFrame();
    m_imgui.Update();
    m_imgui.Render();
//#endif //_DEBUG

    Present();
}


void MyEngine::MyD3DContext::Present()
{
    m_pSwapChain->Present(0, 0);
}

void MyEngine::MyD3DContext::UninitializeScene()
{
    m_pStaticMeshRenderers.clear();
    m_pRigidMeshRenderers.clear();
    m_pSkinningMeshRenderers.clear();
    AssimpConverter::Release();

    m_sceneObjects.clear();
    m_pDirectionalLightT.reset();
    m_pVertexBuffer = nullptr;
    m_pIndexBuffer = nullptr;
    m_pSkyBoxVertexBuffer = nullptr;
    m_pSkyBoxIndexBuffer = nullptr;
    m_pConstantBuffer = nullptr;
    m_pCubeInputLayout = nullptr;
    m_pSkyBoxInputLayout = nullptr;
    m_pVertexShader = nullptr;
    m_pPixelShader = nullptr;
    m_pPixelShaderSolid = nullptr;
    m_pSkyBoxVShader = nullptr;
    m_pSkyBoxPShader = nullptr;
    m_pCubeTextureRV = nullptr;
    m_pCubeNormalMapRV = nullptr;
    m_pCubeSpecularMapRV = nullptr;
    m_pSkyBoxTextureRV = nullptr;
    m_pShadowTex = nullptr;
    m_pShadowSRV = nullptr;
    m_pShadowDSV = nullptr;
    m_pShadowMapVS = nullptr;
    m_pShadowSampler = nullptr;
    m_pShadowMapRasterizerState = nullptr;
}

MyEngine::MyD3DContext::~MyD3DContext()
{
    Material::ReleaseDefaultShaders();
#ifdef _DEBUG
    m_imgui.Uninitialize();
#endif //_DEBUG
    m_pContext = nullptr;
    m_pd3dDevice1 = nullptr;
    m_pd3dDevice = nullptr;
    m_pSwapChain1 = nullptr;
    m_pSwapChain = nullptr;
    m_pRenderTargetView = nullptr;
    m_pDepthStencil = nullptr;
    m_pDepthStencilView = nullptr;
    m_hWnd = nullptr;
    m_pDefRasterizerState = nullptr;
    m_pClockWiseRasterizerState = nullptr;
    m_pBlendState = nullptr;
    m_pOpaqueState = nullptr;
    m_pTransparentState = nullptr;
}

HRESULT MyEngine::MyD3DContext::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

void MyEngine::MyD3DContext::Resize(UINT width, UINT height)
{
    if (!m_pSwapChain || !m_pd3dDevice || !m_pContext)
        return;

    // 멤버 변수 업데이트
    m_width = width;
    m_height = height;

    m_pCamera->SetAspectRatio((float)width / (float)height);

    // 현재 렌더 타겟이 설정되어 있다면 해제
    m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

    // 기존 렌더 타겟 뷰 해제
    m_pRenderTargetView.Reset();

    // 기존 뎁스 스텐실 뷰 해제
    m_pDepthStencilView.Reset();

    // 스왑 체인 버퍼 크기 재조정
    HRESULT hr = m_pSwapChain->ResizeBuffers(
        1,                  // 버퍼 개수
        width,              // 새로운 너비
        height,             // 새로운 높이
        DXGI_FORMAT_UNKNOWN, // 포맷 유지
        0                   // 플래그
    );

    if (FAILED(hr)) {
        // 오류 처리 로직 추가
        OutputDebugStringA("스왑체인의 버퍼 사이즈를 바꾸는 데 실패했습니다.\n");
        return;
    }

    // 새로운 렌더 타겟 뷰 생성
    ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr)) {
        OutputDebugStringA("백버퍼를 얻는 것을 실패했습니다.\n");
        return;
    }

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, m_pRenderTargetView.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("렌더 타겟 뷰를 생성을 실패했습니다.\n");
        return;
    }

    // 새로운 뎁스 스텐실 버퍼 및 뷰 생성
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_pDepthStencil.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("뎁스 스텐실 버퍼를 생성하는 데 실패했습니다.\n");
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("뎁스 스텐실 뷰를 생성하는 데 실패했습니다.\n");
        return;
    }

    // 렌더 타겟 다시 설정
    m_pContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

    // 뷰포트 업데이트
    m_vp.Width = (FLOAT)width;
    m_vp.Height = (FLOAT)height;
    m_vp.MinDepth = 0.0f;
    m_vp.MaxDepth = 1.0f;
    m_vp.TopLeftX = 0;
    m_vp.TopLeftY = 0;
}

