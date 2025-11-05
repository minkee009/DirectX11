#include "Mesh.h"
#include <stdexcept>

void MyEngine::Mesh::CreateBuffers(ID3D11Device* pDevice)
{
    if (!m_pVertexBuffer)
    {
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

        if (FAILED(hr)) {
            m_pVertexBuffer->Release();
            throw std::runtime_error("Failed to create vertex buffer.");
        }
    }

    if (!m_pIndexBuffer)
    {
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
        
        if (FAILED(hr)) {
            m_pVertexBuffer->Release();
            m_pIndexBuffer->Release();
            throw std::runtime_error("Failed to create index buffer.");
        }
    }
}

MyEngine::Mesh::Mesh(const std::vector<VertexType>& vertices, const std::vector<UINT>& indices, const BoundingBox& bbox)
    : m_vertices(vertices)
    , m_indices(indices)
    , m_bbox(bbox)
{
}

MyEngine::Mesh::~Mesh()
{
	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;
}

void MyEngine::Mesh::Bind(ID3D11DeviceContext* ctx)
{
    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    ctx->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    ctx->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}
