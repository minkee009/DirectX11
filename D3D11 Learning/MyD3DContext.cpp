#include "TimeManager.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "InputLayoutManager.h"
#include "MyD3DContext.h"
#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>
#include <wincodec.h>
#include <dxgi1_6.h>
#include <random>

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

    m_supportHDR = CheckHDRSupport();

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
        sd.Format = m_supportHDR ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
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
        sd.BufferDesc.Format = m_supportHDR ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = m_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(m_pd3dDevice.Get(), &sd, m_pSwapChain.GetAddressOf());
    }

    ComPtr<IDXGISwapChain3> spSwapChain3;
    m_pSwapChain.As<IDXGISwapChain3>(&spSwapChain3);

    // hdr 지원인 경우 설정할 것들
    if (m_supportHDR &&
        FAILED(spSwapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
    {
		throw std::runtime_error("Failed to set swap chain color space.");
    }

    m_exposure = m_supportHDR ? 45.0f : 0.53f;

    // 이 튜토리얼 코드는 풀스크린 스왑체인을 관리하지 않음, 따라서 ALT+ENTER 단축키를 제외시킴
    dxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (!m_dxgiDevice)
    {
        ComPtr<IDXGIDevice> dxgiDevice;
        if (SUCCEEDED(m_pd3dDevice.As(&dxgiDevice)))
        {
            dxgiDevice.As(&m_dxgiDevice); // IDXGIDevice3로 업캐스트
        }
    }

    if (FAILED(hr))
        return false;

    // 렌더 타겟 뷰 생성
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return false;

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, m_pBackBufferRTV.GetAddressOf());
    pBackBuffer->Release();
    if (FAILED(hr))
        return false;

    // 씬 컬러 텍스쳐 생성
	D3D11_TEXTURE2D_DESC descTex;
	ZeroMemory(&descTex, sizeof(descTex));
	descTex.Width = width;
	descTex.Height = height;
	descTex.MipLevels = 1;
	descTex.ArraySize = 1;
	descTex.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	descTex.SampleDesc.Count = 1;
	descTex.SampleDesc.Quality = 0;
	descTex.Usage = D3D11_USAGE_DEFAULT;
	descTex.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	descTex.CPUAccessFlags = 0;
	descTex.MiscFlags = 0;
	hr = m_pd3dDevice->CreateTexture2D(&descTex, NULL, m_pSceneColorTex.GetAddressOf());
	if (FAILED(hr))
		return false;

	// 포스트 프로세스 렌더 타겟 뷰 생성
	hr = m_pd3dDevice->CreateRenderTargetView(m_pSceneColorTex.Get(), NULL, m_pSceneColorRTV.GetAddressOf());
	if (FAILED(hr))
		return false;

	// 포스트 프로세스 쉐이더 리소스 뷰 생성
	hr = m_pd3dDevice->CreateShaderResourceView(m_pSceneColorTex.Get(), NULL, m_pSceneColorSRV.GetAddressOf());
	if (FAILED(hr))
		return false;

    // 포스트 프로세스 상수 버퍼 생성
	D3D11_BUFFER_DESC bd = {};
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(PostProcessCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = m_pd3dDevice->CreateBuffer(&bd, NULL, m_pPostProcessCB.GetAddressOf());
	if (FAILED(hr))
		return false;

    bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(BlurCB);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = m_pd3dDevice->CreateBuffer(&bd, NULL, m_pBlurCB.GetAddressOf());
    if (FAILED(hr))
        return false;

    // Bright 
    hr = m_pd3dDevice->CreateTexture2D(&descTex, nullptr, m_pBrightTex.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("Bloom Bright 텍스쳐를 생성하는 데 실패했습니다.\n");
        return false;
    }
    hr = m_pd3dDevice->CreateRenderTargetView(m_pBrightTex.Get(), nullptr, m_pBrightRTV.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("Bloom Bright 렌더 타겟 뷰를 생성하는 데 실패했습니다.\n");
        return false;
    }
    // 리소스 뷰 재생성
    m_pBrightSRV = nullptr;
    hr = m_pd3dDevice->CreateShaderResourceView(m_pBrightTex.Get(), nullptr, m_pBrightSRV.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("Bloom Bright 셰이더 리소스 뷰를 생성하는 데 실패했습니다.\n");
        return false;
    }

	// Blur Temp 텍스쳐 생성
	hr = m_pd3dDevice->CreateTexture2D(&descTex, nullptr, m_pBlurTempTex.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Blur Temp 텍스쳐를 생성하는 데 실패했습니다.\n");
		return false;
	}
	hr = m_pd3dDevice->CreateRenderTargetView(m_pBlurTempTex.Get(), nullptr, m_pBlurTempRTV.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Blur Temp 렌더 타겟 뷰를 생성하는 데 실패했습니다.\n");
		return false;
	}
	// 리소스 뷰 재생성
	m_pBlurTempSRV = nullptr;
	hr = m_pd3dDevice->CreateShaderResourceView(m_pBlurTempTex.Get(), nullptr, m_pBlurTempSRV.GetAddressOf());
	if (FAILED(hr)) {  
		OutputDebugStringA("Bloom Blur Temp 셰이더 리소스 뷰를 생성하는 데 실패했습니다.\n");
		return false;
	}



    // 뎁스 스텐실 텍스쳐 생성
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, NULL, m_pDepthStencilTex.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 뎁스 스텐실 뷰 생성
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencilTex.Get(), &descDSV, m_pDepthStencilView.GetAddressOf());
    if (FAILED(hr))
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
    depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // SRV로 사용할 포맷
    depthSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;
    hr = m_pd3dDevice->CreateShaderResourceView(m_pDepthStencilTex.Get(), &depthSrvDesc, m_pDepthStencilSRV.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("뎁스 셰이더 리소스 뷰를 생성하는 데 실패했습니다.\n");
        return false;
    }

    m_pContext->OMSetRenderTargets(1, m_pSceneColorRTV.GetAddressOf(), m_pDepthStencilView.Get());

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

    // Geometry Pass용 Blend State 설정
    D3D11_BLEND_DESC deferredBlendDesc = {};
    deferredBlendDesc.AlphaToCoverageEnable = FALSE;
    // 모든 RTV에 대해 독립적인 설정을 적용하겠다고 명시
    deferredBlendDesc.IndependentBlendEnable = FALSE;

    // 단일 Target 설정
    deferredBlendDesc.RenderTarget[0].BlendEnable = FALSE; // 블렌딩 비활성화
    deferredBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    deferredBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    deferredBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    deferredBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    deferredBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    deferredBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    deferredBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    m_pd3dDevice->CreateBlendState(&deferredBlendDesc, &m_pGeometryBlendState);

    D3D11_BLEND_DESC desc = {};
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;

    D3D11_RENDER_TARGET_BLEND_DESC& rt = desc.RenderTarget[0];
    rt.BlendEnable = TRUE;

    // RGB
    rt.SrcBlend = D3D11_BLEND_ONE;
    rt.DestBlend = D3D11_BLEND_ONE;
    rt.BlendOp = D3D11_BLEND_OP_ADD;

    // Alpha (보통 사용 안 함)
    rt.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt.DestBlendAlpha = D3D11_BLEND_ONE;
    rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;

    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    m_pd3dDevice->CreateBlendState(&desc, &m_pAdditiveBlendState);

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

    InitGBufferTex();

    D3DCTX::TextureManager::Get()->StartUp(m_pd3dDevice.Get(), m_pContext.Get());
    D3DCTX::ShaderManager::Get()->StartUp(m_pd3dDevice.Get(), m_pContext.Get());
    D3DCTX::InputLayoutManager::Get()->StartUp(m_pd3dDevice.Get(), m_pContext.Get());

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

    InitDefferedRenderpassBuffer();
    InitSkyBox();
    InitShadowMapTex();
    InitBRDFEnvironment();

    if (!m_pBVHTree)
        m_pBVHTree = std::make_unique<StaticBVH>();

    D3D11_BUFFER_DESC pickCBDesc;
    ZeroMemory(&pickCBDesc, sizeof(D3D11_BUFFER_DESC));
    pickCBDesc.Usage = D3D11_USAGE_DYNAMIC;
    pickCBDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pickCBDesc.ByteWidth = sizeof(PickingCB);
    pickCBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_pd3dDevice->CreateBuffer(&pickCBDesc, nullptr, m_pPickingCB.GetAddressOf());

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R32_UINT;
    desc.SampleDesc.Count = 1;          // 
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;          // 
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    hr = m_pd3dDevice->CreateTexture2D(&desc, nullptr, m_pPickingStagingTex.GetAddressOf());

    //상수 버퍼 생성
    //D3D11_BUFFER_DESC cbDesc;
    //ZeroMemory(&cbDesc, sizeof(cbDesc));
    //cbDesc.Usage = D3D11_USAGE_DEFAULT;
    //cbDesc.ByteWidth = sizeof(MyConstantBuffer);
    //cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //cbDesc.CPUAccessFlags = 0;

    //hr = m_pd3dDevice->CreateBuffer(&cbDesc, nullptr, m_pConstantBuffer.GetAddressOf());
    //if (FAILED(hr))
    //    return false;

    //cbDesc = {};
    //cbDesc.Usage = D3D11_USAGE_DEFAULT;
    //cbDesc.ByteWidth = sizeof(OutlineCB);
    //cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //cbDesc.CPUAccessFlags = 0;

    //hr = m_pd3dDevice->CreateBuffer(&cbDesc, nullptr, m_pOutlineCB.GetAddressOf());
    //if (FAILED(hr))
    //    return false;

    //cbDesc = {};
    //cbDesc.Usage = D3D11_USAGE_DEFAULT;
    //cbDesc.ByteWidth = sizeof(GradientCB);
    //cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //cbDesc.CPUAccessFlags = 0;

    //hr = m_pd3dDevice->CreateBuffer(&cbDesc, nullptr, m_pGradientCB.GetAddressOf());
    //if (FAILED(hr))
    //    return false;

    //LUT 텍스쳐 생성 (Todo: Texture Manager에게 위임시키기)
    ScratchImage image;

    //텍스쳐 로드
    //hr = LoadFromDDSFile(L"Resources/Textures/seafloor.dds", DDS_FLAGS_NONE, nullptr, image);
    //hr = LoadFromWICFile(L"Resources/Textures/Lut.png", WIC_FLAGS_NONE, nullptr, image);
    //if (FAILED(hr))
    //    return false;

    //hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), m_pLUTSRV.GetAddressOf());
    //if (FAILED(hr))
    //    return false;

    //카메라 생성
    m_pCamera = std::make_unique<Camera>();
    m_pCamera->GetTransform()->SetWorldPosition(8.69f, 11.2f, 19.9f);
    m_pCamera->GetTransform()->SetWorldEulerRotation(-16.1f, 11.4f, 0.0f);
    m_pCamera->SetAspectRatio((float)m_width, (float)m_height);

    m_pDirectionalLightT = std::make_unique<Transform>();
    m_pDirectionalLightT->SetLocalEulerRotation({ -42.8f,74.5f,0 });
    m_pDirectionalLightT->SetLocalScale({ 1,1,1 });

    //오브젝트 생성
    m_sceneObjects.push_back(std::make_unique<Transform>());
    m_sceneObjects.push_back(std::make_unique<Transform>());
    m_sceneObjects.push_back(std::make_unique<Transform>());

    auto obj1 = m_sceneObjects[0].get();
    obj1->SetWorldPosition(0, 0, 0);

    auto obj2 = m_sceneObjects[1].get();
    obj2->SetWorldPosition(4.950f, 0.250f, 0.0f);

    auto obj3 = m_sceneObjects[2].get();
    obj3->SetWorldPosition(-3.8f, 0.25f, 0.0f);

    AssimpConverter::SetLoadMaterialType(AssimpConverter::LoadMaterialType::BRDF);
    AssimpConverter::SetLoadMaterialProperties(AssimpConverter::LoadMaterialProperties::All);
    m_meshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/Ground.fbx"));
    AssimpConverter::SetLoadMaterialProperties(AssimpConverter::LoadMaterialProperties::OnlyBaseColor);
    m_meshRenderers.push_back(AssimpConverter::LoadSkinningMeshRendererFromFile("Resources/Models/SkinningTest.fbx"));
    AssimpConverter::SetLoadMaterialProperties(AssimpConverter::LoadMaterialProperties::All);
    m_meshRenderers.push_back(AssimpConverter::LoadStaticMeshRendererFromFile("Resources/Models/char.fbx"));

    // renderpass 
    // 0 : shadow map
    // 1 : outline
    // 2 : scene draw

    // Ground.fbx setting
    m_meshRenderers[0]->SetPassCheckKeyword("IsBRDF");

    // skinningTest.fbx setting
    m_meshRenderers[1]->SetPassCheckKeyword("IsBRDF");

    // char.fbx setting
    m_meshRenderers[2]->SetPassCheckKeyword("IsBRDF");

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

    // BVH build
    for (size_t i = 0; i < m_sceneObjects.size(); i++)
    {
        BoundingBox transformedBBox{};

        m_meshRenderers[i]->GetBBox().Transform(transformedBBox, m_sceneObjects[i]->GetWorldMatrix());

        m_bboxRegistry.push_back(transformedBBox);
    }

    m_pBVHTree->Build(m_bboxRegistry);

    // 시드 설정
    std::mt19937 rng(static_cast<unsigned int>(std::time(0)));

    // 분포 정의
    std::uniform_real_distribution<float> posXZDist(-8.2f, 8.2f); // 광원의 초기 위치 범위
    std::uniform_real_distribution<float> posYDist(3.0f, 6.2f); // 광원의 초기 위치 범위
    std::uniform_real_distribution<float> colorDist(0.3f, 1.0f); // 색상 (어두운 색상 방지)
    std::uniform_real_distribution<float> intensityDist(15.0f, 20.0f); // 밝기 (15.0 ~ 20.0)
    std::uniform_real_distribution<float> rangeDist(1.0f, 7.5f); // 범위 (1.0 ~ 7.5)
    std::uniform_real_distribution<float> offsetDist(0.0f, 100.0f); // 애니메이션 오프셋

   
    m_pointLights.resize(NUM_POINT_LIGHTS);

    for (size_t i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        PointLight& light = m_pointLights[i];

        // 1. 위치 랜덤 초기화
        light.Position = light.InitialPosition = Vector3(
            posXZDist(rng), // X 위치
            posYDist(rng), // Y 위치 (또는 Z, Y가 위쪽 축인 경우)
            posXZDist(rng)  // Z 위치
        );

        // 2. 색상 랜덤 초기화 (R, G, B)
        light.Color = Vector3(
            colorDist(rng),
            colorDist(rng),
            colorDist(rng)
        );

        // 3. 밝기 (Intensity: 1.0 ~ 15.0)
        light.Intensity = intensityDist(rng);

        // 4. 범위 (Range: 1.0 ~ 8.0)
        light.Range = rangeDist(rng);

        // 5. 애니메이션 오프셋 (반딧불이 애니메이션의 위상차)
        light.AnimationTimeOffset = offsetDist(rng);
    }

    return true;
}

void MyEngine::MyD3DContext::Update()
{
    m_pCamera->InputUpdate(TIME_GET_DELTA());

    auto lightFwd = m_pDirectionalLightT->GetWorldMatrix().Forward();
    m_pDirectionalLightT->SetLocalPosition(lightFwd * -SHADOW_MAP_DEPTH);

    for (auto& renderer : m_meshRenderers)
    {
        if (auto skinnedMesh = dynamic_cast<SkinningMeshRenderer*>(renderer.get()))
        {
            skinnedMesh->AnimationUpdate();
            skinnedMesh->MatrixUpdate();
        }
    }
    m_mouseLeftClick = false;
    auto mouse = DirectX::Mouse::Get().GetState();
    static bool lastMouseLeft = false;

    bool currMouseLeft = mouse.leftButton; // 매 프레임 갱신

    m_mouseLeftClick = (!lastMouseLeft && currMouseLeft);

    lastMouseLeft = currMouseLeft;

    m_mouseXY = { mouse.x, mouse.y };

    if (m_mouseLeftClick && !m_imgui.GetIsHovered())
    {
        m_pContext->CopyResource(m_pPickingStagingTex.Get(), m_pGBufferTextures[6].Get());

        D3D11_MAPPED_SUBRESOURCE mapped;
        m_pContext->Map(m_pPickingStagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped);

        uint32_t* row = (uint32_t*)((uint8_t*)mapped.pData + m_mouseXY.y * mapped.RowPitch);
        m_currentPickedID = row[m_mouseXY.x] - 1;

        m_pContext->Unmap(m_pPickingStagingTex.Get(), 0);
    }



    // 광원의 애니메이션 속도 및 범위 설정
    const float MOVEMENT_SPEED = 2.0f; // 광원이 움직이는 속도
    const float MOVEMENT_RADIUS = 0.5f; // 광원이 초기 위치 주변에서 움직이는 반경 (반딧불이 효과)

    auto s_time = TimeManager::Get()->GetTime();

    for (PointLight& light : m_pointLights)
    {
        // 각 광원에 대해 애니메이션을 적용
        float timeX = s_time * MOVEMENT_SPEED + light.AnimationTimeOffset * 1.0f;
        float timeY = s_time * MOVEMENT_SPEED + light.AnimationTimeOffset * 1.5f;
        float timeZ = s_time * MOVEMENT_SPEED + light.AnimationTimeOffset * 2.0f;

        float offsetX = MOVEMENT_RADIUS * std::sin(timeX);
        float offsetY = MOVEMENT_RADIUS * std::cos(timeY * 0.7f); // Y축은 약간 다르게 움직이게
        float offsetZ = MOVEMENT_RADIUS * std::sin(timeZ * 1.3f);

        light.Position.x = light.InitialPosition.x + offsetX;
        light.Position.y = light.InitialPosition.y + offsetY;
        light.Position.z = light.InitialPosition.z + offsetZ;
    }
}

bool MyEngine::MyD3DContext::CreateConstantBuffer(
    ID3D11Device* device, 
    UINT size, 
    D3D11_USAGE usage, 
    UINT cpuAccess, 
    ComPtr<ID3D11Buffer>& outBuffer)
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = usage;
    desc.ByteWidth = (size + 15) & ~15;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = cpuAccess;

    return SUCCEEDED(device->CreateBuffer(&desc, nullptr, outBuffer.GetAddressOf()));
}

bool MyEngine::MyD3DContext::InitDefferedRenderpassBuffer()
{
    if (!CreateConstantBuffer(m_pd3dDevice.Get(), sizeof(ObjectMatCB),
        D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, m_pObjectMatBuffer))
    {
        OutputDebugStringA("Failed to create ObjectMatBuffer\n");
        return false;
    }

    if (!CreateConstantBuffer(m_pd3dDevice.Get(), sizeof(CameraCB),
        D3D11_USAGE_DEFAULT, 0, m_pCameraBuffer))
    {
        OutputDebugStringA("Failed to create CameraBuffer\n");
        return false;
    }

    if (!CreateConstantBuffer(m_pd3dDevice.Get(), sizeof(DirectionalLightCB),
        D3D11_USAGE_DEFAULT, 0, m_pDirectionalLightBuffer))
    {
        OutputDebugStringA("Failed to create DirectionalLightBuffer\n");
        return false;
    }

    if (!CreateConstantBuffer(m_pd3dDevice.Get(), sizeof(PointLightCB),
        D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, m_pPointLightBuffer))
    {
        OutputDebugStringA("Failed to create PointLightBuffer\n");
        return false;
    }

    if (!CreateConstantBuffer(m_pd3dDevice.Get(), sizeof(PBRDebugCB),
        D3D11_USAGE_DEFAULT, 0, m_pPBRDebugBuffer))
    {
        OutputDebugStringA("Failed to create PBRDebugBuffer\n");
        return false;
    }

    return true;
}

void MyEngine::MyD3DContext::UninitDefferedRenderpassBuffer()
{
    m_pObjectMatBuffer = nullptr;
    m_pCameraBuffer = nullptr;
    m_pDirectionalLightBuffer = nullptr;
    m_pPointLightBuffer = nullptr;
    m_pPBRDebugBuffer = nullptr;
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
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

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

bool MyEngine::MyD3DContext::InitBRDFEnvironment()
{
    //텍스쳐 로드
	HRESULT hr = S_OK;
    ScratchImage image;

    DirectX::TexMetadata metadata;
    hr = LoadFromDDSFile(L"Resources/Textures/cubemap2Brdf.dds", DDS_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
        return false;

    hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), metadata, m_pBRDFLUTSRV.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = LoadFromDDSFile(L"Resources/Textures/cubemap2EnvHDR.dds", DDS_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
        return false;
    if (!metadata.IsCubemap())
    {
        MessageBox(nullptr,
            L"BRDF Diffuse 텍스쳐가 큐브맵이 아닙니다.", L"오류", MB_OK);
        return false;
    }
    hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), metadata, m_pEnvSRV.GetAddressOf());
    if (FAILED(hr))
        return false;

	hr = LoadFromDDSFile(L"Resources/Textures/cubemap2DiffuseHDR.dds", DDS_FLAGS_NONE, &metadata, image);
	if (FAILED(hr))
		return false;
    if (!metadata.IsCubemap())
    {
        MessageBox(nullptr,
            L"BRDF Diffuse 텍스쳐가 큐브맵이 아닙니다.", L"오류", MB_OK);
        return false;
    }
	hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), metadata, m_pIrradianceSRV.GetAddressOf());
    if (FAILED(hr))
        return false;

	hr = LoadFromDDSFile(L"Resources/Textures/cubemap2SpecularHDR.dds", DDS_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
		return false;
    if (!metadata.IsCubemap())
    {
        MessageBox(nullptr,
            L"BRDF Specular 텍스쳐가 큐브맵이 아닙니다.", L"오류", MB_OK);
        return false;
	}

    hr = CreateShaderResourceView(m_pd3dDevice.Get(), image.GetImages(), image.GetImageCount(), metadata, m_pPrefilteredEnvSRV.GetAddressOf());
    if (FAILED(hr))
		return false;

    // 바인딩
	m_pContext->PSSetShaderResources(20, 1, m_pBRDFLUTSRV.GetAddressOf()); 
	m_pContext->PSSetShaderResources(21, 1, m_pIrradianceSRV.GetAddressOf());
	m_pContext->PSSetShaderResources(22, 1, m_pPrefilteredEnvSRV.GetAddressOf());
	m_pContext->PSSetShaderResources(23, 1, m_pEnvSRV.GetAddressOf());

    return true;
}

bool MyEngine::MyD3DContext::InitGBufferTex()
{
    HRESULT hr = S_OK;

    UINT width = m_width;
    UINT height = m_height;

    for (size_t i = 0; i < GBUFFER_TEX_SIZE; i++)
    {
        // 텍스처 2D 디스크립션 설정
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = m_GBufferFormats[i];
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        // Geometry Pass의 출력(RTV)과 Lighting Pass의 입력(SRV)으로 사용
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        // 텍스처 생성
        hr = m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, m_pGBufferTextures[i].GetAddressOf());
        if (FAILED(hr)) return false;

        // 렌더 타겟 뷰 (RTV) 생성
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = texDesc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        hr = m_pd3dDevice->CreateRenderTargetView(m_pGBufferTextures[i].Get(), &rtvDesc, m_pGBufferRTV[i].GetAddressOf());
        if (FAILED(hr)) return false;

        // 셰이더 리소스 뷰 (SRV) 생성
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        hr = m_pd3dDevice->CreateShaderResourceView(m_pGBufferTextures[i].Get(), &srvDesc, m_pGBufferSRV[i].GetAddressOf());
        if (FAILED(hr)) return false;
    }

    return true;
}

void MyEngine::MyD3DContext::UninitGBufferTex()
{
    for (size_t i = 0; i < GBUFFER_TEX_SIZE; i++)
    {
        m_pGBufferSRV[i] = nullptr;
        m_pGBufferRTV[i] = nullptr;
        m_pGBufferTextures[i] = nullptr;
    }
}

void MyEngine::MyD3DContext::DrawSkyBox()
{

}

void MyEngine::MyD3DContext::DrawShadowMap()
{

}

void MyEngine::MyD3DContext::DrawSkeleton(Transform& t, SkinningMeshRenderer& renderer)
{
    auto& bones = renderer.GetMesh().GetBones();
    auto& currentBonePoses = renderer.GetBonePoses();

    // 본 드로우
    for (auto& sbone : bones)
    {
        if (sbone.parentIndex == -1)
            continue;

        auto& bone_pose = currentBonePoses[sbone.index];

        auto finMat = bone_pose.model.Transpose() * t.GetWorldMatrix();
        auto startPos = Vector3::Transform(Vector3::Zero, finMat);

        auto finMat2 = t.GetWorldMatrix();
        auto endPos = Vector3::Transform(Vector3::Zero, finMat2);

        auto& parent_bone_pose = currentBonePoses[bones[sbone.parentIndex == 0 ? sbone.index : sbone.parentIndex].index];

        finMat2 = parent_bone_pose.model.Transpose() * t.GetWorldMatrix();
        endPos = Vector3::Transform(Vector3::Zero, finMat2);

        BoundingSphere sphr{ startPos,0.025f };
        DX::Draw(m_batch.get(), sphr, Colors::LightGreen);
        DX::DrawRay(m_batch.get(), startPos, endPos - startPos, false, Colors::LightGreen);
    }

    // 본 경계박스 드로우
    for (auto& sbone : bones)
    {
        if (sbone.parentIndex == -1 || !sbone.hasVertex)
            continue;

        auto bone_center = sbone.bbox.Center;
        auto bone_extend = sbone.bbox.Extents;

        auto& bone_pose = currentBonePoses[sbone.index];
        bone_center = Vector3::Transform(bone_center, bone_pose.model.Transpose());
        bone_center = Vector3::Transform(bone_center, t.GetLocalMatrix());

        auto bone_rot = Quaternion::CreateFromRotationMatrix(bone_pose.model.Transpose());
        bone_rot = bone_rot * t.GetLocalRotation();

        bone_extend = Vector3{ bone_extend.x * t.GetLocalScale().x, bone_extend.y * t.GetLocalScale().y, bone_extend.z * t.GetLocalScale().z };
        BoundingOrientedBox obb = { bone_center, bone_extend, bone_rot };
        DX::Draw(m_batch.get(), obb, Colors::Aqua);
    }
}

void MyEngine::MyD3DContext::CreateSkinningRenderer(const Vector3& pos)
{
    m_sceneObjects.push_back(std::make_unique<Transform>());
    m_sceneObjects.back()->SetWorldPosition(pos);

    AssimpConverter::SetLoadMaterialType(AssimpConverter::LoadMaterialType::BRDF);
    AssimpConverter::SetLoadMaterialProperties(AssimpConverter::LoadMaterialProperties::OnlyBaseColor);
    m_meshRenderers.push_back(AssimpConverter::LoadSkinningMeshRendererFromFile("Resources/Models/SkinningTest.fbx"));

    m_meshRenderers.back()->SetPassForceChangeVS(0, D3DCTX::ShaderManager::Get()->GetCommonVertexShader_SkinningBone());
    m_meshRenderers.back()->SetPassCheckKeyword("IsBRDF");
}

void MyEngine::MyD3DContext::Clear()
{
    //float ClearColor[4] = { 0.0f, 0.9f, 0.6f, 1.0f }; // RGBA
    float ClearColorAlbedo[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // RGBA
    float ClearColorZero[4] = { 0,0,0,0 };
    float ClearColorNormal[4] = { 0.0f, 0.0f, 1.0f, 0.0f }; // (R, G, B, A)

    m_pContext->ClearRenderTargetView(m_pSceneColorRTV.Get(), ClearColorAlbedo);
    m_pContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    for (size_t i = 0; i < GBUFFER_TEX_SIZE; ++i)
    {
        float* selectClearCol;

        if (i == 1) // GBuffer #2: Normal
        {
            selectClearCol = ClearColorNormal;
        }
        else if (i == 2) // GBuffer #3: Albedo
        {
            selectClearCol = ClearColorAlbedo;
        }
        else
        {
            // Position, Metallic, Roughness 등 (대부분 0.0 또는 특정 기본값)
            selectClearCol = ClearColorZero;
        }

        m_pContext->ClearRenderTargetView(m_pGBufferRTV[i].Get(), selectClearCol);
    }

	m_pContext->ClearRenderTargetView(m_pBrightRTV.Get(), ClearColorZero);
	m_pContext->ClearRenderTargetView(m_pBlurTempRTV.Get(), ClearColorZero);    
}

void MyEngine::MyD3DContext::Present()
{
    m_pSwapChain->Present(1, 0);
}

void MyEngine::MyD3DContext::UninitializeScene()
{
    m_meshRenderers.clear();
    AssimpConverter::Release();

	m_pIrradianceSRV = nullptr;
	m_pPrefilteredEnvSRV = nullptr;
	m_pBRDFLUTSRV = nullptr;
    m_pEnvSRV = nullptr;

    m_pBrightTex = nullptr;
    m_pBrightRTV = nullptr;
    m_pBrightSRV = nullptr;

    m_pBlurTempTex = nullptr;
	m_pBlurTempRTV = nullptr;
	m_pBlurTempSRV = nullptr;

    m_pBlurCB = nullptr;

    m_pPickingStagingTex = nullptr;
    m_pPickingCB = nullptr;

    UninitDefferedRenderpassBuffer();

    m_sceneObjects.clear();
    m_pDirectionalLightT.reset();
    m_pSkyBoxVertexBuffer = nullptr;
    m_pSkyBoxIndexBuffer = nullptr;
    m_pConstantBuffer = nullptr;
    m_pLUTSRV = nullptr;
    m_pSkyBoxInputLayout = nullptr;
    m_pSkyBoxVShader = nullptr;
    m_pSkyBoxPShader = nullptr;
    m_pSkyBoxTextureRV = nullptr;
    m_pDebugDrawIL = nullptr;
    m_pShadowTex = nullptr;
    m_pShadowSRV = nullptr;
    m_pShadowDSV = nullptr;
    m_pShadowMapVS = nullptr;
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
    D3DCTX::InputLayoutManager::Get()->ShutDown();
    D3DCTX::ShaderManager::Get()->ShutDown();
    D3DCTX::TextureManager::Get()->ShutDown();
#ifdef _DEBUG
    m_imgui.Uninitialize();

#endif //_DEBUG
    m_pBackBufferRTV = nullptr;
    m_pDepthStencilTex = nullptr;
    m_pDepthStencilView = nullptr;
    m_pDepthStencilSRV = nullptr;
    m_pSceneColorRTV = nullptr;
    m_pSceneColorSRV = nullptr;
    m_pSceneColorTex = nullptr;
    m_pPostProcessCB = nullptr;

    UninitGBufferTex();

    m_pDefRasterizerState = nullptr;
    m_pClockWiseRasterizerState = nullptr;
    m_pBlendState = nullptr;
    m_pGeometryBlendState = nullptr;
    m_pAdditiveBlendState = nullptr;
    m_pOpaqueState = nullptr;
    m_pTransparentState = nullptr;
    m_pSamplerPoint = nullptr;
    m_pSamplerLinear = nullptr;

    m_dxgiDevice = nullptr;
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

bool MyEngine::MyD3DContext::CheckHDRSupport()
{
    if (!m_pd3dDevice)
        throw std::runtime_error("failed to get d3dDevice, check d3dDevice initalize");

    ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return false;

    UINT i = 0;
    ComPtr<IDXGIOutput> output;

    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(m_pd3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
        return false;

    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter)))
        return false;

    while (adapter->EnumOutputs(i++, &output) != DXGI_ERROR_NOT_FOUND)
    {
        ComPtr<IDXGIOutput6> output6;
        if (SUCCEEDED(output.As(&output6)))
        {
            DXGI_OUTPUT_DESC1 desc = {};
            if (SUCCEEDED(output6->GetDesc1(&desc)))
            {
                // HDR10-like color space 체크 예시
                if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                {
                    //std::wcout << L"HDR10-capable output found\n";
                    return true;
                }
            }
        }
        output = nullptr;
    }
    return false;
}

void MyEngine::MyD3DContext::ResizeGBufferTex(UINT width, UINT height)
{
    for (size_t i = 0; i < GBUFFER_TEX_SIZE; i++)
    {
        m_pGBufferTextures[i] = nullptr;
        m_pGBufferRTV[i] = nullptr;
        m_pGBufferSRV[i] = nullptr;

        // 텍스처 2D 디스크립션 설정
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = m_GBufferFormats[i];
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        // Geometry Pass의 출력(RTV)과 Lighting Pass의 입력(SRV)으로 사용
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        // 텍스처 생성
        m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, m_pGBufferTextures[i].GetAddressOf());

        // 렌더 타겟 뷰 (RTV) 생성
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = texDesc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        m_pd3dDevice->CreateRenderTargetView(m_pGBufferTextures[i].Get(), &rtvDesc, m_pGBufferRTV[i].GetAddressOf());

        // 셰이더 리소스 뷰 (SRV) 생성
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        m_pd3dDevice->CreateShaderResourceView(m_pGBufferTextures[i].Get(), &srvDesc, m_pGBufferSRV[i].GetAddressOf());
    }
}

void MyEngine::MyD3DContext::ForwardRenderPass()
{
    m_pContext->OMSetRenderTargets(1, m_pSceneColorRTV.GetAddressOf(), m_pDepthStencilView.Get());
    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);

    //  <=============== 첫번째 패스(그림자 맵)
    m_currentRenderPassNum = 0;

    MyConstantBuffer cb;
    cb.mWorld = XMMatrixIdentity();

    ID3D11ShaderResourceView* firstPassnullSRVs[12] = { nullptr };
    m_pContext->PSSetShaderResources(0, 12, firstPassnullSRVs);

    // 컬러 렌더타겟은 사용 안 함
    m_pContext->PSSetShader(nullptr, nullptr, 0);
    m_pContext->OMSetRenderTargets(0, nullptr, m_pShadowDSV.Get());
    // 깊이 초기화
    m_pContext->ClearDepthStencilView(m_pShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_pContext->OMSetDepthStencilState(m_pOpaqueState.Get(), 0);

    m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
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

    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetInputLayout(D3DCTX::InputLayoutManager::Get()->GetDefaultInputLayout());
    for (size_t i = 0; i < m_sceneObjects.size(); ++i)
    {
        auto& meshRenderer = m_meshRenderers[i];
        auto& obj = m_sceneObjects[i];

        cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
        m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

        meshRenderer->SetEnabledBindMeshes(true);
        meshRenderer->SetEnabledBindMaterials(false);
        meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
        meshRenderer->Draw(m_pContext.Get());
    }

    Clear();

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    m_pContext->PSSetShaderResources(9, 1, nullSRVs);  // 그림자맵 언바인딩

    m_pContext->RSSetState(m_pDefRasterizerState.Get()); //기본 래스터라이저 상태로 복귀
    m_pContext->OMSetRenderTargets(1, m_pSceneColorRTV.GetAddressOf(), m_pDepthStencilView.Get());
    m_pContext->RSSetViewports(1, &m_vp);
    m_pContext->PSSetSamplers(1, 1, m_pShadowSampler.GetAddressOf());
    m_pContext->PSSetShaderResources(10, 1, m_pShadowSRV.GetAddressOf());
    m_pContext->PSSetShaderResources(9, 1, m_pLUTSRV.GetAddressOf());

    cb.mWorld = XMMatrixIdentity();
    cb.mView = XMMatrixTranspose(m_pCamera->GetViewMatrix());
    cb.mProjection = XMMatrixTranspose(m_pCamera->GetProjMatrix());
    cb.CameraPos = m_pCamera->GetTransform()->GetLocalPosition();

    XMStoreFloat3(&cb.vLightPos, XMVectorScale(XMLoadFloat4(&xmLightDir), -1.25f));

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

    OutlineCB olCB = {};
    olCB.Thickness = m_outlineThickness;
    m_pContext->UpdateSubresource(m_pOutlineCB.Get(), 0, nullptr, &olCB, 0, 0);
    m_pContext->VSSetConstantBuffers(4, 1, m_pOutlineCB.GetAddressOf());

    // <<======= 두번째 패스(아웃라인)
    m_currentRenderPassNum = 1;
    m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //스카이박스는 시계방향으로 컬링
    for (size_t i = 0; i < m_sceneObjects.size(); ++i)
    {
        auto& meshRenderer = m_meshRenderers[i];
        auto& obj = m_sceneObjects[i];

        if (!meshRenderer->GetPassCheckKeyword("IsToon"))
            continue;

        cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
        m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pContext->IASetInputLayout(D3DCTX::InputLayoutManager::Get()->GetDefaultInputLayout());

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

        D3DCTX::ShaderManager::Get()->BindOutlineShaders(m_pContext.Get());

        meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
        meshRenderer->SetEnabledBindMeshes(true);
        meshRenderer->SetEnabledBindMaterials(false);
        meshRenderer->Draw(m_pContext.Get());
    }
    m_pContext->RSSetState(m_pDefRasterizerState.Get()); //기본 래스터라이저 상태로 복귀

    GradientCB gradientCB = {};
    gradientCB.intensity = m_gradientIntensity;
    //gradientCB.minY = 0;
    //gradientCB.maxY = 10.0f;
    m_pContext->UpdateSubresource(m_pGradientCB.Get(), 0, nullptr, &gradientCB, 0, 0);
    m_pContext->PSSetConstantBuffers(5, 1, m_pGradientCB.GetAddressOf());

    // <<======= 세번째 렌더패스 (씬 드로우)
    m_currentRenderPassNum = 2;

    Vector3 debugPos1;
    Vector3 debugPos2;
    Vector3 debugPos3;
    bool firstDebugDraw = true;

    for (size_t i = 0; i < m_sceneObjects.size(); ++i)
    {
        auto& meshRenderer = m_meshRenderers[i];
        auto& obj = m_sceneObjects[i];

        cb.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
        m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pContext->IASetInputLayout(D3DCTX::InputLayoutManager::Get()->GetDefaultInputLayout());

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

        if (m_meshRenderers[i]->GetPassCheckKeyword("IsToon"))
        {
            auto gradientBBox = meshRenderer->GetBBox();
            BoundingOrientedBox gradientOBB;
            BoundingOrientedBox::CreateFromBoundingBox(gradientOBB, gradientBBox);

            gradientBBox.Transform(gradientBBox, obj->GetWorldMatrix());
            gradientOBB.Transform(gradientOBB, obj->GetWorldMatrix());

            Vector3 extents = gradientBBox.Extents;

            float dist = -extents.Length();

            if (firstDebugDraw)
            {
                debugPos1 = gradientBBox.Center;
            }

            gradientCB.GradientPos = gradientBBox.Center + (lightFwd * dist);
            if (firstDebugDraw)
            {
                debugPos2 = gradientCB.GradientPos;
            }

            dist = -dist;

            XMVECTOR simdOrigin = XMLoadFloat3(&gradientCB.GradientPos);
            XMVECTOR simdDirection = XMLoadFloat3(&lightFwd);

            if (gradientOBB.Intersects(simdOrigin, simdDirection, dist))
            {
                gradientCB.GradientPos = gradientCB.GradientPos + (lightFwd * dist);
            }

            if (firstDebugDraw)
            {
                debugPos3 = gradientCB.GradientPos;
                firstDebugDraw = false;
            }

        }

        m_pContext->UpdateSubresource(m_pGradientCB.Get(), 0, nullptr, &gradientCB, 0, 0);
        m_pContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
        meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
        meshRenderer->SetEnabledBindMeshes(true);
        meshRenderer->SetEnabledBindMaterials(true);
        meshRenderer->Draw(m_pContext.Get());
    }
}

void MyEngine::MyD3DContext::DefferedRenderPass()
{
    ID3D11ShaderResourceView* nullSRVs[19] = { nullptr };
    m_pContext->PSSetShaderResources(1, 19, nullSRVs);
    m_pContext->VSSetShaderResources(1, 19, nullSRVs);

    ID3D11Buffer* nullBuffers[10] = { nullptr };
    m_pContext->VSSetConstantBuffers(0, 10, nullBuffers);
    m_pContext->PSSetConstantBuffers(0, 10, nullBuffers);

    ObjectMatCB cb_objMat = {};              // fast update
    CameraCB cb_cam = {};
    DirectionalLightCB cb_dirLight = {};
    PointLightCB cb_pointLight = {};         // fast update
    PBRDebugCB cb_pbr_debug = {};
	BlurCB cb_blur = {};
    PickingCB cb_pick = {};

    D3D11_MAPPED_SUBRESOURCE mapped;

    // directional light set-up
    Matrix lightViewMat = m_pDirectionalLightT->GetWorldMatrix().Invert();
    Matrix lightProj = Matrix::CreateOrthographic(50.0f, 50.0f, m_lightProjectNear, m_lightProjectFar);
    Matrix lightViewProj = lightViewMat * lightProj;
    auto lightFwd = m_pDirectionalLightT->GetLocalMatrix().Forward();
    cb_dirLight.Color = { m_lightColor.x, m_lightColor.y, m_lightColor.z };
    cb_dirLight.Direction = XMFLOAT4{ lightFwd.x,lightFwd.y,lightFwd.z,1 };
    cb_dirLight.mLightViewProjection = lightViewProj.Transpose();
    cb_dirLight.Intensity = m_specularStrength;
    cb_dirLight.Position = m_pDirectionalLightT->GetLocalPosition();
    m_pContext->UpdateSubresource(m_pDirectionalLightBuffer.Get(), 0, nullptr, &cb_dirLight, 0, 0);

    // camera set-up
    cb_cam.mView = XMMatrixTranspose(m_pCamera->GetViewMatrix());
    cb_cam.mProjection = XMMatrixTranspose(m_pCamera->GetProjMatrix());
    cb_cam.CameraPos = m_pCamera->GetTransform()->GetLocalPosition();
    m_pContext->UpdateSubresource(m_pCameraBuffer.Get(), 0, nullptr, &cb_cam, 0, 0);

    // pbr debug set-up
    cb_pbr_debug.UseOverride = m_useMatOverride;
    cb_pbr_debug.MetallicOverride = m_diffuseStrength;
    cb_pbr_debug.RoughnessOverride = m_ambientStrength;
    cb_pbr_debug.AmbeintIntensity = m_rimLightStrength;
    m_pContext->UpdateSubresource(m_pPBRDebugBuffer.Get(), 0, nullptr, &cb_pbr_debug, 0, 0);

	// blur cb set-up
	cb_blur.texelSize = XMFLOAT2(1.0f / static_cast<float>(m_width), 1.0f / static_cast<float>(m_height));
    cb_blur.horizontal = 0.0f;
	m_pContext->UpdateSubresource(m_pBlurCB.Get(), 0, nullptr, &cb_blur, 0, 0);

    // pass - 0 : Shadow Cast
    m_currentRenderPassNum = 0;

    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    m_pContext->OMSetRenderTargets(0, nullptr, m_pShadowDSV.Get());
    m_pContext->OMSetDepthStencilState(m_pOpaqueState.Get(), 0);

    m_pContext->ClearDepthStencilView(m_pShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetInputLayout(D3DCTX::InputLayoutManager::Get()->GetDefaultInputLayout());

    m_pContext->RSSetState(m_pShadowMapRasterizerState.Get());
    m_pContext->RSSetViewports(1, &m_shadowViewport);

    m_pContext->VSSetShader(D3DCTX::ShaderManager::Get()->GetShadowCastVertexShader(), nullptr, 0);
    m_pContext->VSSetConstantBuffers(5, 1, m_pObjectMatBuffer.GetAddressOf());
    m_pContext->VSSetConstantBuffers(7, 1, m_pDirectionalLightBuffer.GetAddressOf());

    m_pContext->PSSetShader(nullptr, nullptr, 0);

    for (size_t i = 0; i < m_sceneObjects.size(); ++i)
    {
        auto& meshRenderer = m_meshRenderers[i];
        auto& obj = m_sceneObjects[i];

        bool isSkinned = dynamic_cast<SkinningMeshRenderer*>(meshRenderer.get()) != nullptr;

        cb_objMat.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
        cb_objMat.isSkinnedMesh = isSkinned ? 1 : 0;

        m_pContext->Map(
            m_pObjectMatBuffer.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped
        );

        memcpy(mapped.pData, &cb_objMat, sizeof(ObjectMatCB));

        m_pContext->Unmap(m_pObjectMatBuffer.Get(), 0);


        meshRenderer->SetEnabledBindMeshes(true);
        meshRenderer->SetEnabledBindMaterials(false);
        meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
        meshRenderer->Draw(m_pContext.Get());
    }

    Clear();

    // pass - 1 : Geometry
    m_currentRenderPassNum = 1;
    m_pContext->OMSetBlendState(m_pGeometryBlendState.Get(), NULL, 0xFFFFFFFF);

    ID3D11RenderTargetView* rtvs[GBUFFER_TEX_SIZE] = {};
    for (UINT i = 0; i < GBUFFER_TEX_SIZE; ++i)
        rtvs[i] = m_pGBufferRTV[i].Get();

    m_pContext->OMSetRenderTargets(
        GBUFFER_TEX_SIZE,
        rtvs,
        m_pDepthStencilView.Get()
    );

    m_pContext->PSSetShaderResources(1, 19, nullSRVs);
    m_pContext->VSSetShaderResources(1, 19, nullSRVs);

    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetInputLayout(D3DCTX::InputLayoutManager::Get()->GetDefaultInputLayout());

    m_pContext->RSSetViewports(1, &m_vp);
    m_pContext->RSSetState(m_pDefRasterizerState.Get());

    m_pContext->VSSetConstantBuffers(6, 1, m_pCameraBuffer.GetAddressOf());

    m_pContext->PSSetShader(D3DCTX::ShaderManager().Get()->GetDefferedGeometryPixelShader(), nullptr, 0);
    m_pContext->PSSetConstantBuffers(9, 1, m_pPBRDebugBuffer.GetAddressOf());
    m_pContext->PSSetConstantBuffers(13, 1, m_pPickingCB.GetAddressOf());
    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

    for (size_t i = 0; i < m_sceneObjects.size(); ++i)
    {
        auto& meshRenderer = m_meshRenderers[i];
        auto& obj = m_sceneObjects[i];

        cb_objMat.mWorld = XMMatrixTranspose(obj->GetWorldMatrix());
        cb_pick.RendererID = static_cast<UINT>(i + 1);

        m_pContext->Map(
            m_pObjectMatBuffer.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped
        );

        memcpy(mapped.pData, &cb_objMat, sizeof(ObjectMatCB));
      
        m_pContext->Unmap(m_pObjectMatBuffer.Get(), 0);

        m_pContext->Map(
            m_pPickingCB.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped
        );

        memcpy(mapped.pData, &cb_pick, sizeof(PickingCB));

        m_pContext->Unmap(m_pPickingCB.Get(), 0);

        meshRenderer->SetRenderPassNum(m_currentRenderPassNum);
        meshRenderer->SetEnabledBindMeshes(true);
        meshRenderer->SetEnabledBindMaterials(true);
        meshRenderer->SetExcludeShaderFlag(ExcludeShaderFlag::PixelShader);
        meshRenderer->Draw(m_pContext.Get());
        meshRenderer->SetExcludeShaderFlag(ExcludeShaderFlag::None);
    }

    ID3D11RenderTargetView* nullRTVs[GBUFFER_TEX_SIZE] = { nullptr, };
    m_pContext->OMSetRenderTargets(
        GBUFFER_TEX_SIZE,
        nullRTVs,
        nullptr
    );

    // pass - 2 : Light
    m_currentRenderPassNum = 2;

    m_pContext->PSSetShaderResources(0, 19, nullSRVs);
    m_pContext->VSSetShaderResources(0, 19, nullSRVs);

    // G-Buffer RTV -> SRV Bind
    ID3D11ShaderResourceView* srvs[GBUFFER_TEX_SIZE] = {};
    for (UINT i = 0; i < GBUFFER_TEX_SIZE; ++i)
        srvs[i] = m_pGBufferSRV[i].Get();

    m_pContext->PSSetShaderResources(0, GBUFFER_TEX_SIZE, srvs);
    m_pContext->PSSetShaderResources(10, 1, m_pShadowSRV.GetAddressOf());

    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
    m_pContext->PSSetSamplers(1, 1, m_pShadowSampler.GetAddressOf());
    m_pContext->PSSetConstantBuffers(6, 1, m_pCameraBuffer.GetAddressOf());

    // RTV -> SceneColor
    m_pContext->OMSetRenderTargets(1, m_pSceneColorRTV.GetAddressOf(), nullptr);

    // VS
    ID3D11VertexShader* QuadVS = D3DCTX::ShaderManager::Get()->GetPostProcessingVertexShader();

    m_pContext->OMSetDepthStencilState(nullptr, 0);
    m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //시계로 해야함 <- 왼손좌표계 쿼드
    m_pContext->IASetInputLayout(nullptr);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    m_pContext->VSSetShader(QuadVS, nullptr, 0);

    m_pContext->PSSetSamplers(2, 1, m_pSamplerPoint.GetAddressOf());
    // LightPass 0 : GlobalLight
    {
        ID3D11PixelShader* GlobalLightPassPS = D3DCTX::ShaderManager::Get()->GetDefferedLightPixelShader();
        m_pContext->PSSetShader(GlobalLightPassPS, nullptr, 0);
        m_pContext->PSSetConstantBuffers(7, 1, m_pDirectionalLightBuffer.GetAddressOf());

        m_pContext->Draw(3, 0);
    }

    // LightPass 1 : Additive Point Light
    {
        m_pContext->OMSetBlendState(m_pAdditiveBlendState.Get(), nullptr, 0xFFFFFFFF);

        ID3D11PixelShader* AdditivePointLightPassPS = D3DCTX::ShaderManager::Get()->GetDefferedAdditivePointLightPixelShader();
        m_pContext->PSSetShader(AdditivePointLightPassPS, nullptr, 0);
        m_pContext->PSSetConstantBuffers(8, 1, m_pPointLightBuffer.GetAddressOf());

        // for
        for (size_t i = 0; i < m_drawPointLightCount; ++i)
        {
            auto& pointLight = m_pointLights[i];
            cb_pointLight.Color = pointLight.Color;
            cb_pointLight.Intensity = pointLight.Intensity;
            cb_pointLight.Range = pointLight.Range;
            cb_pointLight.Position = pointLight.Position;

            m_pContext->Map(
                m_pPointLightBuffer.Get(),
                0,
                D3D11_MAP_WRITE_DISCARD,
                0,
                &mapped
            );

            memcpy(mapped.pData, &cb_pointLight, sizeof(PointLightCB));

            m_pContext->Unmap(m_pPointLightBuffer.Get(), 0);
            m_pContext->Draw(3, 0);
        }
    }

    // pass - 3 : SkyBox 
    m_currentRenderPassNum = 3;

    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    m_pContext->OMSetRenderTargets(1, m_pSceneColorRTV.GetAddressOf(), m_pDepthStencilView.Get());

    m_pContext->PSSetShaderResources(1, 1, m_pSkyBoxTextureRV.GetAddressOf());
    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
    m_pContext->PSSetSamplers(2, 1, m_pSamplerPoint.GetAddressOf());
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetInputLayout(m_pSkyBoxInputLayout.Get());
    m_pContext->IASetVertexBuffers(0, 1, m_pSkyBoxVertexBuffer.GetAddressOf(), &m_skyBoxVertexBufferStride, &m_skyBoxVertexBufferOffset);
    m_pContext->IASetIndexBuffer(m_pSkyBoxIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    m_pContext->PSSetShaderResources(0, 1, m_pSkyBoxTextureRV.GetAddressOf());
    m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //스카이박스는 시계방향으로 컬링
    m_pContext->VSSetShader(m_pSkyBoxVShader.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pSkyBoxPShader.Get(), nullptr, 0);
    m_pContext->DrawIndexed(m_skyBoxIndexCount, 0, 0);
    m_pContext->RSSetState(m_pDefRasterizerState.Get()); //기본 래스터라이저 상태로 복귀

    // pass - 4 : Bloom 
	//m_currentRenderPassNum = 4;

	//// SceneColor SRV -> PostProcess SRV Bind
	//ID3D11RenderTargetView* nullRTV[1] = { nullptr };
	//m_pContext->OMSetRenderTargets(1, nullRTV, nullptr);
 //   m_pContext->PSSetShaderResources(0, 1, m_pSceneColorSRV.GetAddressOf());

	//ID3D11VertexShader* BloomVS = D3DCTX::ShaderManager::Get()->GetPostProcessingVertexShader();
	//ID3D11PixelShader* BloomPS = D3DCTX::ShaderManager::Get()->GetBrightnessContrastPixelShader();

	//m_pContext->OMSetDepthStencilState(nullptr, 0);
	//m_pContext->IASetInputLayout(nullptr);
	//m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//m_pContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	//m_pContext->VSSetShader(BloomVS, nullptr, 0);
	//m_pContext->PSSetShader(BloomPS, nullptr, 0);
	//m_pContext->RSSetViewports(1, &m_vp);
	//m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //시계로 해야함 <- 왼손좌표계 쿼드
	//m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
	//m_pContext->OMSetRenderTargets(1, m_pBrightRTV.GetAddressOf(), nullptr);
	//m_pContext->Draw(3, 0);

 //   ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	//m_pContext->PSSetShaderResources(0, 1, nullSRV);  // slot 0 초기화

 //   m_pContext->OMSetRenderTargets(1, nullRTV, nullptr);

	//BloomPS = D3DCTX::ShaderManager::Get()->GetGaussianBlurPixelShader();
	//m_pContext->PSSetShader(BloomPS, nullptr, 0);
	//m_pContext->PSSetShaderResources(0, 1, m_pBrightSRV.GetAddressOf());

	//m_pContext->PSSetConstantBuffers(0, 1, m_pBlurCB.GetAddressOf());
	//m_pContext->OMSetRenderTargets(1, m_pBlurTempRTV.GetAddressOf(), nullptr);
	//m_pContext->Draw(3, 0);

 //   m_pContext->OMSetRenderTargets(1, nullRTV, nullptr);
 //   m_pContext->PSSetShaderResources(0, 1, m_pBlurTempSRV.GetAddressOf());
	//m_pContext->OMSetRenderTargets(1, m_pBrightRTV.GetAddressOf(), nullptr);
 //   cb_blur.horizontal = 1.0f;
	//m_pContext->UpdateSubresource(m_pBlurCB.Get(), 0, nullptr, &cb_blur, 0, 0);
	//m_pContext->Draw(3, 0);

 //   ID3D11ShaderResourceView* nullSRVs2[1] = { nullptr };
	//m_pContext->PSSetShaderResources(0, 1, nullSRVs2);  // slot 0 초기화

 //   // Bloom Add
 //   m_pContext->OMSetRenderTargets(1, m_pBlurTempRTV.GetAddressOf(), nullptr);
 //   ID3D11PixelShader* BloomAddPS = D3DCTX::ShaderManager::Get()->GetBloomCombinePixelShader();
 //   m_pContext->PSSetShader(BloomAddPS, nullptr, 0);
 //   m_pContext->PSSetShaderResources(0, 1, m_pSceneColorSRV.GetAddressOf());
 //   m_pContext->PSSetShaderResources(1, 1, m_pBrightSRV.GetAddressOf());
 //   m_pContext->Draw(3, 0);
 //   m_pContext->OMSetRenderTargets(1, nullRTV, nullptr);
 //   m_pContext->PSSetShaderResources(0, 1, nullSRVs2);  // slot 0 초기화
	//m_pContext->PSSetShaderResources(1, 1, nullSRVs2);  // slot 1 초기화

	// gaussian blur end
 //   ID3D11RenderTargetView* nullRTV[1] = { nullptr };
	//m_pContext->OMSetRenderTargets(1, nullRTV, nullptr);
	//m_pContext->OMSetRenderTargets(1, m_pBrightRTV.GetAddressOf(), nullptr);

	//auto BlurVS = D3DCTX::ShaderManager::Get()->GetPostProcessingVertexShader();
	//auto BlurPS = D3DCTX::ShaderManager::Get()->GetGaussianBlurPixelShader();

	//m_pContext->OMSetDepthStencilState(nullptr, 0);
	//m_pContext->IASetInputLayout(nullptr);
	//m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//m_pContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	//m_pContext->VSSetShader(BlurVS, nullptr, 0);
	//m_pContext->PSSetShader(BlurPS, nullptr, 0);
	//m_pContext->RSSetViewports(1, &m_vp);
	//m_pContext->RSSetState(m_pClockWiseRasterizerState.Get()); //시계로 해야함 <- 왼손좌표계 쿼드
	//m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
	//m_pContext->PSSetConstantBuffers(0, 1, m_pBlurCB.GetAddressOf());
	//// horizontal blur
	//cb_blur.horizontal = 1.0f;
	//m_pContext->UpdateSubresource(m_pBlurCB.Get(), 0, nullptr, &cb_blur, 0, 0);
	//m_pContext->PSSetShaderResources(0, 1, m_pSceneColorSRV.GetAddressOf());
	//m_pContext->Draw(3, 0);
	//// vertical blur
	//cb_blur.horizontal = 0.0f;
	//m_pContext->UpdateSubresource(m_pBlurCB.Get(), 0, nullptr, &cb_blur, 0, 0);
	//m_pContext->OMSetRenderTargets(1, m_pBlurTempRTV.GetAddressOf(), nullptr);
	//m_pContext->PSSetShaderResources(0, 1, m_pBrightSRV.GetAddressOf());
	//m_pContext->Draw(3, 0);

 //   ID3D11ShaderResourceView* nullSRV2[1] = { nullptr };
	//m_pContext->PSSetShaderResources(0, 1, nullSRV2);  // slot 0 초기화
}

void MyEngine::MyD3DContext::Render()
{
    DefferedRenderPass();

    // tone mapping
    ID3D11RenderTargetView* nullRTV[1] = { nullptr };
    m_pContext->OMSetRenderTargets(1, nullRTV, nullptr);

    ID3D11ShaderResourceView* nullSRVs2[19] = { nullptr };
    m_pContext->PSSetShaderResources(1, 19, nullSRVs2);

    ID3D11VertexShader* postProcessingVS = D3DCTX::ShaderManager::Get()->GetPostProcessingVertexShader();
    ID3D11PixelShader* postProcessingPS = D3DCTX::ShaderManager::Get()->GetPostProcessingPixelShader();

    m_pContext->OMSetDepthStencilState(nullptr, 0);
    m_pContext->IASetInputLayout(nullptr);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    m_pContext->VSSetShader(postProcessingVS, nullptr, 0);
    m_pContext->PSSetShader(postProcessingPS, nullptr, 0);

    m_pContext->PSSetShaderResources(0, 1, m_pSceneColorSRV.GetAddressOf());

    PostProcessCB pp_cb = {};
    pp_cb.exposure = m_exposure;
    pp_cb.supportHDR = m_supportHDR ? 1.0f : 0.0f;
    m_pContext->UpdateSubresource(m_pPostProcessCB.Get(), 0, nullptr, &pp_cb, 0, 0);
    m_pContext->PSSetConstantBuffers(0, 1, m_pPostProcessCB.GetAddressOf());

    m_pContext->RSSetViewports(1, &m_vp);
    m_pContext->RSSetState(m_pDefRasterizerState.Get());

    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
    m_pContext->RSSetState(m_pClockWiseRasterizerState.Get());
    m_pContext->OMSetRenderTargets(1, m_pBackBufferRTV.GetAddressOf(), nullptr);
    m_pContext->Draw(3, 0);
    m_pContext->RSSetState(m_pDefRasterizerState.Get());
    ID3D11ShaderResourceView* nullSRV3[1] = { nullptr };
    m_pContext->PSSetShaderResources(0, 1, nullSRV3);

    // debug draw
    if (m_enableDebugDraw)
    {
        if (!m_enableDebugDrawZbuffer)
        {
            m_pContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
            m_pContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
            m_pContext->RSSetState(m_states->CullNone());
        }
        else
        {
            m_pContext->OMSetRenderTargets(1, m_pBackBufferRTV.GetAddressOf(), m_pDepthStencilView.Get());
        }

        m_effect->SetView(m_pCamera->GetViewMatrix());
        m_effect->SetProjection(m_pCamera->GetProjMatrix());
        m_effect->Apply(m_pContext.Get());
        m_pContext->IASetInputLayout(m_pDebugDrawIL.Get());

        m_batch->Begin();

        for (size_t i = 0; i < m_sceneObjects.size(); ++i)
        {
            if (auto skinningMeshRenderer = dynamic_cast<SkinningMeshRenderer*>(m_meshRenderers[i].get()))
            {
                DrawSkeleton(*m_sceneObjects[i].get(), *skinningMeshRenderer);
            }

            if (i == 0) continue;
            auto renderer_AABB = m_meshRenderers[i]->GetBBox();
            auto& obj = m_sceneObjects[i];
            BoundingOrientedBox obb;
            obb.CreateFromBoundingBox(obb, renderer_AABB);
            obb.Transform(obb, obj->GetWorldMatrix());
            DX::Draw(m_batch.get(), obb, Colors::Aqua);
        }

        auto frustum = m_pCamera->GetProjFrustum();
        frustum.Transform(frustum, m_pCamera->GetTransform()->GetWorldMatrix());

        DX::Draw(m_batch.get(), frustum, Colors::GhostWhite);

        for (size_t i = 0; i < m_drawPointLightCount; ++i)
        {
            auto pointLight = m_pointLights[i];
            BoundingSphere sphr1{ pointLight.Position,0.05f };
            BoundingSphere sphr2{ pointLight.Position,pointLight.Range };

            auto debugLightCol = Color(pointLight.Color.x, pointLight.Color.y, pointLight.Color.z, 1.0f);

            DX::Draw(m_batch.get(), sphr1, debugLightCol);
            DX::Draw(m_batch.get(), sphr2, debugLightCol);
        }

        m_batch->End();

        m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    }


    //#ifdef _DEBUG
    m_imgui.BeginFrame();
    m_imgui.Update();
    m_imgui.Render();
    //#endif //_DEBUG

    Present();
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
    m_pBackBufferRTV = nullptr;
    m_pDepthStencilView = nullptr;
    m_pDepthStencilTex = nullptr;

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

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, m_pBackBufferRTV.GetAddressOf());
    if (FAILED(hr)) {
        pBackBuffer = nullptr;
        OutputDebugStringA("렌더 타겟 뷰를 생성을 실패했습니다.\n");
        return;
    }

    // 포스트 프로세스 텍스쳐 및 뷰 재생성
    m_pSceneColorTex = nullptr;
    m_pSceneColorRTV = nullptr;
    D3D11_TEXTURE2D_DESC postProcessTexDesc = {};
    postProcessTexDesc.Width = width;
    postProcessTexDesc.Height = height;
    postProcessTexDesc.MipLevels = 1;
    postProcessTexDesc.ArraySize = 1;
    postProcessTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    postProcessTexDesc.SampleDesc.Count = 1;
    postProcessTexDesc.SampleDesc.Quality = 0;
    postProcessTexDesc.Usage = D3D11_USAGE_DEFAULT;
    postProcessTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    postProcessTexDesc.CPUAccessFlags = 0;
    postProcessTexDesc.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&postProcessTexDesc, nullptr, m_pSceneColorTex.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("포스트 프로세스 텍스쳐를 생성하는 데 실패했습니다.\n");
        return;
    }
    hr = m_pd3dDevice->CreateRenderTargetView(m_pSceneColorTex.Get(), nullptr, m_pSceneColorRTV.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("포스트 프로세스 렌더 타겟 뷰를 생성하는 데 실패했습니다.\n");
        return;
    }

    // 리소스 뷰 재생성
    m_pSceneColorSRV = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC postProcessSRVDesc = {};
    postProcessSRVDesc.Format = postProcessTexDesc.Format;
    postProcessSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    postProcessSRVDesc.Texture2D.MostDetailedMip = 0;
    postProcessSRVDesc.Texture2D.MipLevels = 1;
    hr = m_pd3dDevice->CreateShaderResourceView(m_pSceneColorTex.Get(), &postProcessSRVDesc, m_pSceneColorSRV.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("포스트 프로세스 셰이더 리소스 뷰를 생성하는 데 실패했습니다.\n");
        return;
    }

	// Bloom용 Bright 텍스쳐 및 뷰 재생성 -> post process와 동일한 설정 사용
	m_pBrightTex = nullptr;
	m_pBrightRTV = nullptr;

	hr = m_pd3dDevice->CreateTexture2D(&postProcessTexDesc, nullptr, m_pBrightTex.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Bright 텍스쳐를 생성하는 데 실패했습니다.\n");
		return;
	}
	hr = m_pd3dDevice->CreateRenderTargetView(m_pBrightTex.Get(), nullptr, m_pBrightRTV.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Bright 렌더 타겟 뷰를 생성하는 데 실패했습니다.\n");
		return;
	}
	// 리소스 뷰 재생성
	m_pBrightSRV = nullptr;
	hr = m_pd3dDevice->CreateShaderResourceView(m_pBrightTex.Get(), &postProcessSRVDesc, m_pBrightSRV.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Bright 셰이더 리소스 뷰를 생성하는 데 실패했습니다.\n");
		return;
	}

	m_pBlurTempTex = nullptr;
	m_pBlurTempRTV = nullptr;
	hr = m_pd3dDevice->CreateTexture2D(&postProcessTexDesc, nullptr, m_pBlurTempTex.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Blur Temp 텍스쳐를 생성하는 데 실패했습니다.\n");
		return;
	}
	hr = m_pd3dDevice->CreateRenderTargetView(m_pBlurTempTex.Get(), nullptr, m_pBlurTempRTV.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Blur Temp 렌더 타겟 뷰를 생성하는 데 실패했습니다.\n");
		return;
	}
	// 리소스 뷰 재생성
	m_pBlurTempSRV = nullptr;
	hr = m_pd3dDevice->CreateShaderResourceView(m_pBlurTempTex.Get(), &postProcessSRVDesc, m_pBlurTempSRV.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringA("Bloom Blur Temp 셰이더 리소스 뷰를 생성하는 데 실패했습니다.\n");
		return;
	}


    // 새로운 뎁스 스텐실 버퍼 및 뷰 생성
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_pDepthStencilTex.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("뎁스 스텐실 버퍼를 생성하는 데 실패했습니다.\n");
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencilTex.Get(), &descDSV, m_pDepthStencilView.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("뎁스 스텐실 뷰를 생성하는 데 실패했습니다.\n");
        return;
    }

    m_pDepthStencilSRV = nullptr; // 멤버 변수 선언 가정
    D3D11_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
    depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // SRV로 사용할 포맷
    depthSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;
    hr = m_pd3dDevice->CreateShaderResourceView(m_pDepthStencilTex.Get(), &depthSrvDesc, m_pDepthStencilSRV.GetAddressOf());
    if (FAILED(hr)) {
        OutputDebugStringA("뎁스 셰이더 리소스 뷰를 생성하는 데 실패했습니다.\n");
        return;
    }

    // GBuffer 리사이즈
    ResizeGBufferTex(width, height);

    // 렌더 타겟 다시 설정
    m_pContext->OMSetRenderTargets(1, m_pSceneColorRTV.GetAddressOf(), m_pDepthStencilView.Get());

    // 뷰포트 업데이트
    m_vp.Width = (FLOAT)width;
    m_vp.Height = (FLOAT)height;
    m_vp.MinDepth = 0.0f;
    m_vp.MaxDepth = 1.0f;
    m_vp.TopLeftX = 0;
    m_vp.TopLeftY = 0;
}

