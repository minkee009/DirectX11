#include "MyD3DContext.h"
#include <wrl/client.h> // Microsoft::WRL::ComPtr
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
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

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
            D3D11_SDK_VERSION, &p_d3dDevice, &m_featureLevel, &p_immediateContext);

        if (hr == E_INVALIDARG)
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                D3D11_SDK_VERSION, &p_d3dDevice, &m_featureLevel, &p_immediateContext);
        }

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return false;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = p_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
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

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2)
    {
        // DirectX 11.1 or later
        hr = p_d3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&p_d3dDevice1));
        if (SUCCEEDED(hr))
        {
            (void)p_immediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&p_immediateContext));
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd(p_d3dDevice, m_hWnd, &sd, nullptr, nullptr, &p_swapChain1);
        if (SUCCEEDED(hr))
        {
            hr = p_swapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&p_swapChain));
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
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

        hr = dxgiFactory->CreateSwapChain(p_d3dDevice, &sd, &p_swapChain);
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
        return false;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = p_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return false;

    hr = p_d3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &p_renderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return false;

    p_immediateContext->OMSetRenderTargets(1, &p_renderTargetView, nullptr);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    p_immediateContext->RSSetViewports(1, &vp);

    return true;
}

void MyEngine::MyD3DContext::Clear()
{
    float ClearColor[4] = { 0.0f, 0.9f, 0.6f, 1.0f }; // RGBA

    p_immediateContext->ClearRenderTargetView(p_renderTargetView, ClearColor);
}

void MyEngine::MyD3DContext::Render()
{
	Clear();

    p_immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    p_immediateContext->IASetVertexBuffers(0, 1, &p_vertexBuffer, &m_vertexBufferStride, &m_vertexBufferOffset);
    p_immediateContext->IASetInputLayout(p_vertexLayout);
    p_immediateContext->VSSetShader(p_vertexShader, nullptr, 0);
    p_immediateContext->PSSetShader(p_pixelShader, nullptr, 0);

    p_immediateContext->IASetIndexBuffer(p_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    p_immediateContext->DrawIndexed(m_indexCount, 0, 0);

	Present();
}

void MyEngine::MyD3DContext::Present()
{
    p_swapChain->Present(0, 0);
}

bool MyEngine::MyD3DContext::InitializeScene()
{
    HRESULT hr = S_OK;

    //컴파일 정보 저장용 객체
    ID3DBlob* pVSBlob = nullptr;

    //셰이더 컴파일링
    hr = CompileShaderFromFile(L"Tutorial02_VS.hlsl", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"정점 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = p_d3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &p_vertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return false;
    }

    //인풋 레이아웃 (셰이더 코드 바인딩) 설정
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    //인풋 레이아웃 생성
    hr = p_d3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &p_vertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return false;

    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"Tutorial02_PS.hlsl", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
        return false;
    }

    hr = p_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &p_pixelShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return false;

    //정점 정의
    MyVertex vertices[] = {
        {XMFLOAT3(-0.5f,-0.5f,0.5f), XMFLOAT4(1.0f,0.0f,0.0f,1.0f)},    // v0
        {XMFLOAT3(-0.5f,0.5f,0.5f), XMFLOAT4(0.0f,1.0f,0.0f,1.0f)},     // v1
        {XMFLOAT3(0.5f,0.5f,0.5f), XMFLOAT4(0.0f,0.0f,1.0f,1.0f)},      // v2
        {XMFLOAT3(0.5f,-0.5f,0.5f), XMFLOAT4(1.0f,1.0f,0.0f,1.0f)},     // v3
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
    hr = p_d3dDevice->CreateBuffer(&vbDesc, &vbData, &p_vertexBuffer);

    if (FAILED(hr))
        return false;

    m_vertexBufferStride = sizeof(MyVertex);
    m_vertexBufferOffset = 0;

    //인덱스 정의
    UINT indices[] = { 0, 1, 2, 3, 0, 2 };

    //인덱스 버퍼 정의
    D3D11_BUFFER_DESC bufferDesc;
    m_indexCount = ARRAYSIZE(indices);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * m_indexCount;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    //인덱스 버퍼 생성
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = indices;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    hr = p_d3dDevice->CreateBuffer(&bufferDesc, &InitData, &p_indexBuffer);
    if (FAILED(hr))
        return false;

    return true;
}

void MyEngine::MyD3DContext::UninitializeScene()
{
    if (p_vertexBuffer)
        p_vertexBuffer->Release();
    if (p_vertexLayout)
        p_vertexLayout->Release();
    if (p_vertexShader)
        p_vertexShader->Release();
    if (p_pixelShader)
        p_pixelShader->Release();
}

MyEngine::MyD3DContext::~MyD3DContext()
{
	if (p_immediateContext) p_immediateContext->ClearState();
	if (p_renderTargetView) p_renderTargetView->Release();
	if (p_swapChain1) p_swapChain1->Release();
	if (p_swapChain) p_swapChain->Release();
	if (p_immediateContext) p_immediateContext->Release();
	if (p_d3dDevice1) p_d3dDevice1->Release();
	if (p_d3dDevice) p_d3dDevice->Release();

	p_immediateContext = NULL;
    p_d3dDevice1 = NULL;
	p_d3dDevice = NULL;
    p_swapChain1 = NULL;
	p_swapChain = NULL;
	p_renderTargetView = NULL;
	m_hWnd = NULL;
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
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
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

