#include "InputLayoutManager.h"
#include "ShaderManager.h"
#include "Material.h"

void MyEngine::D3DCTX::InputLayoutManager::StartUp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    ID3DBlob* pVSBlob = ShaderManager::Get()->GetBlinnPhongVSBlob();
    if (!pVSBlob) return; // 셰이더 바이트 코드를 얻지 못했다면 종료

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
    HRESULT hr = pDevice->CreateInputLayout(layout, numElements,
        pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(),
        m_pDefaultInputLayout.GetAddressOf());
    if (FAILED(hr))
        return;
}

void MyEngine::D3DCTX::InputLayoutManager::ShutDown()
{
    m_pDefaultInputLayout = nullptr;
}