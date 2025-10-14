#include "Mesh.h"
#include <stdexcept>

MyEngine::Mesh::Mesh(const std::vector<VertexType>& vertices, const std::vector<UINT>& indices, ID3D11Device* device)
    : vertices(vertices)
    , indices(indices)
{
	//정점 버퍼 초기화
    HRESULT hr;

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = static_cast<UINT>(sizeof(VertexType) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = &vertices[0];

    hr = device->CreateBuffer(&vbd, &initData, m_pVertexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        m_pVertexBuffer->Release();
        throw std::runtime_error("Failed to create vertex buffer.");
    }

    D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;

    initData.pSysMem = &indices[0];

    hr = device->CreateBuffer(&ibd, &initData, m_pIndexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        m_pVertexBuffer->Release();
        m_pIndexBuffer->Release();
        throw std::runtime_error("Failed to create index buffer.");
    }
}
