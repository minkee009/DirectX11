#include "Mesh.h"
#include <stdexcept>

MyEngine::Mesh::Mesh(const std::vector<VertexType>& vertices, const std::vector<UINT>& indices, const AABB& aabb)
    : m_vertices(vertices)
    , m_indices(indices)
    , m_aabb(aabb)
{
}

void MyEngine::Mesh::Bind(ID3D11DeviceContext* ctx)
{
    if (!m_pVertexBuffer)
    {
        ID3D11Device* pDevice;
        ctx->GetDevice(&pDevice);

        HRESULT hr;

        D3D11_BUFFER_DESC vbd;
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.ByteWidth = static_cast<UINT>(sizeof(VertexType) * m_vertices.size());
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = 0;
        vbd.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = &m_vertices[0];

        hr = pDevice->CreateBuffer(&vbd, &initData, m_pVertexBuffer.GetAddressOf());
        pDevice->Release();
        if (FAILED(hr)) {
            m_pVertexBuffer->Release();
            throw std::runtime_error("Failed to create vertex buffer.");
        }
    }

    if (!m_pIndexBuffer)
    {
        ID3D11Device* pDevice;
        ctx->GetDevice(&pDevice);

        HRESULT hr;

        D3D11_BUFFER_DESC ibd;
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * m_indices.size());
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.CPUAccessFlags = 0;
        ibd.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = &m_indices[0];

        hr = pDevice->CreateBuffer(&ibd, &initData, m_pIndexBuffer.GetAddressOf());
        pDevice->Release();
        if (FAILED(hr)) {
            m_pVertexBuffer->Release();
            m_pIndexBuffer->Release();
            throw std::runtime_error("Failed to create index buffer.");
        }
    }

    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    ctx->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    ctx->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}
