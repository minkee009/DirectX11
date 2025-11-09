#include "Time.h"
#include "MyD3DContext.h"
#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>
#include <wincodec.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#pragma comment(lib, "dxguid.lib")

#include "StaticMeshRenderer.h"
#include "RigidMeshRenderer.h"
#include "SkinningMeshRenderer.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

namespace MyEngine::GlobalLogic::ToonShader
{
    bool GetLutOnePixelColor(const wchar_t* file, BYTE outRGBA[4])
    {
        IWICImagingFactory* factory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
            return false;

        IWICBitmapDecoder* decoder = nullptr;
        if (FAILED(factory->CreateDecoderFromFilename(file, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)))
            return false;

        IWICBitmapFrameDecode* frame = nullptr;
        decoder->GetFrame(0, &frame);

        IWICFormatConverter* converter = nullptr;
        factory->CreateFormatConverter(&converter);
        converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);

        // (0,0) 픽셀만 읽기
        WICRect rect = { 0,0,1,1 };
        converter->CopyPixels(&rect, 4, 4, outRGBA);

        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return true;
    }
    
    BYTE GetLutOnePixelR(const wchar_t* file)
    {
        BYTE outCol[4];
        if (GetLutOnePixelColor(file, &outCol[0]))
        {
            return outCol[0];
        }
        return 0;
    }
}

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
            ID3D11DeviceContext1* pContext1 = nullptr;
            hr = m_pContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&pContext1));
            if (SUCCEEDED(hr))
            {
                m_pContext.Attach(pContext1);
            }
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.BufferCount = 2;

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
        sd.BufferCount = 2;
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
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerLinear.GetAddressOf());
    if (FAILED(hr))
        return false;

    sampDesc = {};
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerPoint.GetAddressOf());
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

    cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(OutlineCB);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;

    hr = m_pd3dDevice->CreateBuffer(&cbDesc, nullptr, m_pOutlineCB.GetAddressOf());
    if (FAILED(hr))
        return false;

    cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(GradientCB);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;

    hr = m_pd3dDevice->CreateBuffer(&cbDesc, nullptr, m_pGradientCB.GetAddressOf());
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

    int createColRow = 10;
    int createObjCount = createColRow * createColRow;
    float createGap = 4.7f;
    float createStartPos = (createColRow - 1) * createGap * 0.5f;

    for (int i = 0; i < createColRow; ++i)
    {
        for (int j = 0; j < createColRow; ++j)
        {
            float posX = -createStartPos + createGap * i;
            float posZ = -createStartPos + createGap * j;

            auto pTransform = std::make_unique<Transform>();
            pTransform->SetLocalPosition({ posX, 0.0f, posZ });
            pTransform->SetLocalScale({ 0.0276f, 0.0276f ,0.0276f });

            m_sceneObjects.emplace_back(std::move(pTransform));
        }
    }

    createColRow = 10;
    createObjCount = createColRow * createColRow;
    createGap = 4.7f;
    createStartPos = (createColRow - 1) * createGap * 0.5f + (createGap * 0.5f);

    for (int i = 0; i < createColRow; ++i)
    {
        for (int j = 0; j < createColRow; ++j)
        {
            float posX = -createStartPos + createGap * i;
            float posZ = -createStartPos + createGap * j;

            auto pTransform = std::make_unique<Transform>();
            pTransform->SetLocalPosition({ posX, 0.0f, posZ });
            pTransform->SetLocalScale({ 0.0276f, 0.0276f ,0.0276f });

            m_sceneObjects.emplace_back(std::move(pTransform));
        }
    }

    // 땅바닥
    auto groundObj = m_sceneObjects[0].get();
    groundObj->SetWorldPosition(0, 0, 0);
    groundObj->SetLocalScale(0.05f, 0.05f, 0.05f);

    AssimpConverter::SetLoadMaterialType(AssimpConverter::LoadMaterialType::BlinnPhong);

    m_meshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/Ground.fbx"));
    m_meshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/Character.fbx"));
    m_meshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/Tree.fbx"));

    // renderpass 
    // 0 : shadow map
    // 1 : outline
    // 2 : scene draw

    m_meshRenderers[0]->SetPassForceChangeVS(0, Material::GetBlinnPhongVertexShader());
    m_meshRenderers[0]->SetPassForceChangePS(0, nullptr);
    m_meshRenderers[1]->SetPassForceChangeVS(0, Material::GetBlinnPhongVertexShader());
    m_meshRenderers[1]->SetPassForceChangePS(0, nullptr);
    m_meshRenderers[2]->SetPassForceChangeVS(0, Material::GetBlinnPhongVertexShader());
    m_meshRenderers[2]->SetPassForceChangePS(0, Material::GetBlinnPhongShadowMapPixelShader());

    //// skinningTest.fbx setting
    //m_meshRenderers[0]->SetPassForceChangeVS(0, Material::GetBlinnPhongVertexShader_SkinningBone());
    //m_meshRenderers[0]->SetPassForceChangeVS(1, Material::GetOutlineVertexShader_SkinningBone());

    //// Miyu_Akey_Rigging.obj setting
    //m_meshRenderers[1]->SetPassExcludedMeshes(0, { 1,5 }); // shadow pass -> { 1, 5 } exclude :: built-in ModelFile outline meshes
    //m_meshRenderers[1]->SetPassExcludedMeshes(1, { 1,5 }); // outline pass -> { 1, 5 } exclude  :: built-in ModelFile outline meshes
    //m_meshRenderers[1]->SetPassForceChangeVS(0, Material::GetBlinnPhongVertexShader());

    //// Ground.fbx setting
    //m_meshRenderers[2]->SetPassForceChangeVS(0, Material::GetBlinnPhongVertexShader());

    //// zeldaPosed001.fbx setting
    //m_meshRenderers[3]->SetPassForceChangeVS(0, Material::GetBlinnPhongVertexShader());

    //BVH Setting
    m_pBVHTree = std::make_unique<BVH>();

    for (int i = 0; i < m_sceneObjects.size(); ++i)
    {
        BoundingBox bbox;

        if (i == 0)
        {
            bbox = m_meshRenderers[0]->GetBBox();
        }
        else if (i < 100)
        {
            bbox = m_meshRenderers[1]->GetBBox();
        }
        else
        {
            bbox = m_meshRenderers[2]->GetBBox();
        }

        bbox.Transform(bbox, m_sceneObjects[i]->GetWorldMatrix());
        m_bboxRegistry.push_back(bbox);
    }

    m_pBVHTree->Build(m_bboxRegistry);
    
    // DebugDraw
    m_states = std::make_unique<CommonStates>(m_pd3dDevice.Get());
    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_pContext.Get());
    m_effect = std::make_unique<BasicEffect>(m_pd3dDevice.Get());
    m_effect->SetVertexColorEnabled(true);
    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        m_pd3dDevice->CreateInputLayout(
            VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
            shaderByteCode, byteCodeLength,
            m_pDebugDrawIL.ReleaseAndGetAddressOf());
    }

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

    hr = m_pd3dDevice->CreateSamplerState(&samplerDesc, m_pShadowSampler.GetAddressOf());
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
    hr = m_pd3dDevice->CreateRasterizerState(&rd, m_pShadowMapRasterizerState.GetAddressOf());
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

void MyEngine::MyD3DContext::DrawSkyBox()
{

}

void MyEngine::MyD3DContext::DrawCube()
{
}

void MyEngine::MyD3DContext::DrawShadowMap()
{

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

    //  <=============== 첫번째 패스(그림자 맵)
    m_currentRenderPassNum = 0;

    MyConstantBuffer cb;
    cb.mWorld = XMMatrixIdentity();

    ID3D11ShaderResourceView* firstPassnullSRVs[7] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    m_pContext->PSSetShaderResources(0, 7, firstPassnullSRVs);

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

    cb.rimLightStr = m_rimLightStrength;

    static BYTE lowlutPixel = MyEngine::GlobalLogic::ToonShader::GetLutOnePixelR(L"Resources/Textures/Lut.png");
    cb.lowLut = pow((float)lowlutPixel / 255.0f, 2.2f); // 정규화 후 감마 색상표로 전환 

    float colorMapA = cb.lowLut - m_diffuseGradientStrength;
    float colorMapB = cb.lowLut;
    cb.diffGradientDistHalf = abs(colorMapB - colorMapA) * 0.5f;
    cb.diffGradientDepth = colorMapB - cb.diffGradientDistHalf;

    m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

    //그림자맵 패스 끄기 (BVH 순수 성능 테스트)
    //{
    //    auto& meshRenderer = m_meshRenderers[0];
    //    auto& obj = m_sceneObjects[0];

    //    cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
    //    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    //    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //    m_pContext->IASetInputLayout(m_pCubeInputLayout.Get()); //나중에 수정하기 -> VertexType IL로 꼭 변경

    //    meshRenderer->SetEnabledBindMeshes(true);
    //    meshRenderer->SetEnabledBindMaterials(false);
    //    meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
    //    meshRenderer->Draw(m_pContext.Get());
    //}

    //for (size_t i = 1; i < m_sceneObjects.size(); ++i)
    //{
    //    auto& meshRenderer = i < 100 ? m_meshRenderers[1] : m_meshRenderers[2];
    //    auto& obj = m_sceneObjects[i];

    //    cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
    //    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    //    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //    m_pContext->IASetInputLayout(m_pCubeInputLayout.Get()); //나중에 수정하기 -> VertexType IL로 꼭 변경

    //    meshRenderer->SetEnabledBindMeshes(true);
    //    meshRenderer->SetEnabledBindMaterials(true);
    //    meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
    //    meshRenderer->Draw(m_pContext.Get());
    //}
    
    Clear();

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    m_pContext->PSSetShaderResources(6, 1, nullSRVs);  // 그림자맵 언바인딩

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
    m_pContext->PSSetSamplers(2, 1, m_pSamplerPoint.GetAddressOf());
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

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    m_pContext->PSSetShaderResources(0, 1, nullSRV);  // slot 0 초기화

    int modelIdx = 0;

    //OutlineCB olCB = {};
    //olCB.Thickness = m_outlineThickness;
    //m_pContext->UpdateSubresource(m_pOutlineCB.Get(), 0, nullptr, &olCB, 0, 0);
    //m_pContext->VSSetConstantBuffers(4, 1, m_pOutlineCB.GetAddressOf());

    //// <<======= 두번째 패스(아웃라인)
    //m_currentRenderPassNum = 1;
    //m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //스카이박스는 시계방향으로 컬링
    //for (size_t i = 0; i < m_sceneObjects.size(); ++i)
    //{
    //    auto& meshRenderer = m_meshRenderers[i];
    //    auto& obj = m_sceneObjects[i];

    //    cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
    //    m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    //    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //    m_pContext->IASetInputLayout(m_pCubeInputLayout.Get());

    //    m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    //    m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

    //    Material::BindOutlineShaders(m_pContext.Get());

    //    meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
    //    meshRenderer->SetEnabledBindMeshes(true);
    //    meshRenderer->SetEnabledBindMaterials(false);
    //    meshRenderer->Draw(m_pContext.Get());
    //}
    //m_pContext->RSSetState(m_pDefRasterizerState.Get()); //기본 래스터라이저 상태로 복귀
    //
    //GradientCB gradientCB = {};
    //gradientCB.ColorTop = m_gradientColorTop;
    //gradientCB.ColorBottom = m_gradientColorBottom;
    //gradientCB.intensity = m_gradientIntensity;
    ////gradientCB.minY = 0;
    ////gradientCB.maxY = 10.0f;
    //m_pContext->UpdateSubresource(m_pGradientCB.Get(), 0, nullptr, &gradientCB, 0, 0);
    //m_pContext->PSSetConstantBuffers(5, 1, m_pGradientCB.GetAddressOf());

    // <<======= 두번째 렌더패스 (씬 드로우)
    m_currentRenderPassNum = 2;
    std::vector<size_t> culledObjIndices;
    auto frustum = m_pCamera->GetProjFrustum();
    frustum.Transform(frustum, m_pCamera->GetTransform()->GetWorldMatrix());
    if (m_usingBVH)
    {
        m_pBVHTree->Search(m_pBVHTree->GetRootIdx(), m_bboxRegistry, frustum, culledObjIndices);
        for (auto& obj_idx : culledObjIndices)
        {
            MeshRenderer* meshRenderer;
            if (obj_idx == 0)
                meshRenderer = m_meshRenderers[0].get();
            else if (obj_idx < 100)
                meshRenderer = m_meshRenderers[1].get();
            else
                meshRenderer = m_meshRenderers[2].get();

            auto& obj = m_sceneObjects[obj_idx];

            cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
            m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

            m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_pContext->IASetInputLayout(m_pCubeInputLayout.Get());

            m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
            m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

            m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
            meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
            meshRenderer->SetEnabledBindMeshes(true);
            meshRenderer->SetEnabledBindMaterials(true);
            meshRenderer->Draw(m_pContext.Get());
        }
    }
    else
    {
        for (size_t i = 0; i < m_sceneObjects.size(); ++i)
        {
            MeshRenderer* meshRenderer;
            if (i == 0)
                meshRenderer = m_meshRenderers[0].get();
            else if (i < 100)
                meshRenderer = m_meshRenderers[1].get();
            else
                meshRenderer = m_meshRenderers[2].get();

            auto& obj = m_sceneObjects[i];

            cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
            m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

            m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_pContext->IASetInputLayout(m_pCubeInputLayout.Get());

            m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
            m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

            m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
            meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
            meshRenderer->SetEnabledBindMeshes(true);
            meshRenderer->SetEnabledBindMaterials(true);
            meshRenderer->Draw(m_pContext.Get());
        }
    }
    

    //auto skinnedMesh = static_cast<SkinningMeshRenderer*>(m_meshRenderers[0].get());
    //skinnedMesh->AnimationUpdate();
    //skinnedMesh->MatrixUpdate();

    //debug draw
    m_mappedIdx = m_usingBVH ? culledObjIndices.size() : m_sceneObjects.size();
    if (m_enableDebugDraw)
    {
        if (!m_enableDebugDrawZbuffer)
        {
            m_pContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
            m_pContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
            m_pContext->RSSetState(m_states->CullNone());
        }

        m_effect->SetView(m_pCamera->GetViewMatrix());
        m_effect->SetProjection(m_pCamera->GetProjMatrix());
        m_effect->Apply(m_pContext.Get());
        m_pContext->IASetInputLayout(m_pDebugDrawIL.Get());

        m_batch->Begin();

        if(m_usingBVH)
            for (size_t i = 0; i < culledObjIndices.size(); ++i)
            {
                auto renderer_AABB = m_bboxRegistry[culledObjIndices[i]];
                DX::Draw(m_batch.get(), renderer_AABB, Colors::Aqua);
            }
        else
            for (size_t i = 0; i < m_sceneObjects.size(); ++i)
            {
                MeshRenderer* meshRenderer;
                if (i == 0)
                    meshRenderer = m_meshRenderers[0].get();
                else if (i < 100)
                    meshRenderer = m_meshRenderers[1].get();
                else
                    meshRenderer = m_meshRenderers[2].get();
                auto renderer_AABB = meshRenderer->GetBBox();
                auto& obj = m_sceneObjects[i];
                BoundingOrientedBox obb;
                obb.CreateFromBoundingBox(obb, renderer_AABB);
                obb.Transform(obb, obj->GetWorldMatrix());
                DX::Draw(m_batch.get(), obb, Colors::Aqua);
            }
        //auto mapped_AABB = m_bboxRegistry[m_pBVHTree->GetMappedIdx(m_mappedIdx)];
        //DX::Draw(m_batch.get(), mapped_AABB, Colors::Aqua);
        // 
        //auto& bones = skinnedMesh->GetSkinningMesh().GetBones();

        //// 본 드로우
        //for (auto& sbone : bones)
        //{
        //    if (sbone.parentIndex == -1)
        //        continue;

        //    auto finMat = sbone.model.Transpose() * m_sceneObjects[0]->GetWorldMatrix();
        //    auto startPos = Vector3::Transform(Vector3::Zero, finMat);

        //    auto finMat2 = m_sceneObjects[0]->GetWorldMatrix();
        //    auto endPos = Vector3::Transform(Vector3::Zero, finMat2);

        //    finMat2 = bones[sbone.parentIndex == 0 ? sbone.index : sbone.parentIndex].model.Transpose() * m_sceneObjects[0]->GetWorldMatrix();
        //    endPos = Vector3::Transform(Vector3::Zero, finMat2);

        //    BoundingSphere sphr{ startPos,0.025f };
        //    DX::Draw(m_batch.get(), sphr, Colors::LightGreen);
        //    DX::DrawRay(m_batch.get(), startPos, endPos - startPos, false, Colors::LightGreen);
        //}

        //// 본 경계박스 드로우
        //for (auto& sbone : bones)
        //{
        //    if (sbone.parentIndex == -1)
        //        continue;

        //    auto bone_center = sbone.bbox.Center;
        //    auto bone_extend = sbone.bbox.Extents;

        //    bone_center = Vector3::Transform(bone_center, sbone.model.Transpose());
        //    bone_center = Vector3::Transform(bone_center, m_sceneObjects[0]->GetWorldMatrix());

        //    auto bone_rot = Quaternion::CreateFromRotationMatrix(sbone.model.Transpose());
        //    bone_rot = bone_rot * m_sceneObjects[0]->GetLocalRotation();

        //    bone_extend = Vector3{ bone_extend.x * m_sceneObjects[0]->GetLocalScale().x, bone_extend.y * m_sceneObjects[0]->GetLocalScale().y, bone_extend.z * m_sceneObjects[0]->GetLocalScale().z };
        //    BoundingOrientedBox obb = { bone_center, bone_extend, bone_rot };
        //    DX::Draw(m_batch.get(), obb, Colors::Aqua);
        //}

        /*BoundingSphere sphere = { debugPos1, 0.125f };
        DX::Draw(m_batch.get(), sphere, Colors::Red);

        BoundingSphere sphere2 = { debugPos2, 0.125f };
        DX::Draw(m_batch.get(), sphere2, Colors::Orange);

        BoundingSphere sphere3 = { debugPos3, 0.125f };
        DX::Draw(m_batch.get(), sphere3, Colors::Magenta);*/

		//DX::DrawRay(m_batch.get(), debugPos1, debugPos2 - debugPos1, false, Colors::Yellow);

        DX::Draw(m_batch.get(), frustum, Colors::GhostWhite);

        m_batch->End();
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
    m_meshRenderers.clear();
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
    m_pDebugDrawIL = nullptr;
    m_pShadowTex = nullptr;
    m_pShadowSRV = nullptr;
    m_pShadowDSV = nullptr;
    m_pShadowSampler = nullptr;
    m_pShadowMapRasterizerState = nullptr;
    m_pOutlineCB = nullptr;
    m_pGradientCB = nullptr;
    if (m_pContext)
    {
        m_pContext->ClearState();
        m_pContext->Flush();
    }

    m_batch.reset();      // PrimitiveBatch 해제
    m_effect.reset();     // BasicEffect 해제
    m_states.reset();     // CommonStates 해제 (이게 2개의 refcount 원인!)
    m_pDebugDrawIL = nullptr;
}

MyEngine::MyD3DContext::~MyD3DContext()
{
    Material::ReleaseDefaultShaders();
    Material::ReleaseBlinnPhongShaders();
#ifdef _DEBUG
    m_imgui.Uninitialize();

#endif //_DEBUG
    m_pRenderTargetView = nullptr;
    m_pDepthStencilView = nullptr;
    m_pDepthStencil = nullptr;

    m_pDefRasterizerState = nullptr;
    m_pClockWiseRasterizerState = nullptr;
    m_pBlendState = nullptr;
    m_pOpaqueState = nullptr;
    m_pTransparentState = nullptr;
    m_pSamplerPoint = nullptr;
    m_pSamplerLinear = nullptr;

    m_pContext = nullptr;

    m_pSwapChain1 = nullptr;
    m_pSwapChain = nullptr;

#ifdef _DEBUG
    // 7. Device should be released LAST
    m_pd3dDevice1 = nullptr;
    m_pd3dDevice = nullptr;

    // 8. Debug output AFTER device release
    {
        HMODULE dxgidebugdll = GetModuleHandleW(L"dxgidebug.dll");
        if (dxgidebugdll)
        {
            decltype(&DXGIGetDebugInterface) GetDebugInterface =
                reinterpret_cast<decltype(&DXGIGetDebugInterface)>(
                    GetProcAddress(dxgidebugdll, "DXGIGetDebugInterface"));

            if (GetDebugInterface)
            {
                IDXGIDebug* debug;
                if (SUCCEEDED(GetDebugInterface(IID_PPV_ARGS(&debug))))
                {
                    OutputDebugStringW(L"Starting Live Direct3D Object Dump:\r\n");
                    debug->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL);
                    OutputDebugStringW(L"Completed Live Direct3D Object Dump.\r\n");
                    debug->Release();
                }
            }
        }
    }
#else
    m_pd3dDevice1 = nullptr;
    m_pd3dDevice = nullptr;
#endif
    m_hWnd = nullptr;
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

    // 기존 화면 렌더타겟뷰를 모두 해제
    m_pRenderTargetView.Reset();
    m_pDepthStencilView.Reset();
    m_pDepthStencil.Reset();

    // 스왑 체인 버퍼 크기 재조정
    HRESULT hr = m_pSwapChain->ResizeBuffers(
        0,                  // 버퍼 개수
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
        pBackBuffer.Reset();
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

