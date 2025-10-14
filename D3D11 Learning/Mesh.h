#pragma once
#include "VertexType.h"
#include <vector>
#include <assimp/mesh.h>
#include <string>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <d3d11.h>

using namespace Microsoft::WRL;

namespace MyEngine
{
	class Mesh
	{
	public:
		std::vector<VertexType> vertices;
		std::vector<UINT> indices;
		Mesh(const std::vector<VertexType>& vertices, const std::vector<UINT>& indices, ID3D11Device* device);
		inline ID3D11Buffer* GetVertexBuffer() { return m_pVertexBuffer.Get(); }
		inline ID3D11Buffer* GetIndexBuffer() { return m_pIndexBuffer.Get(); }
	private:
		ComPtr<ID3D11Buffer> m_pVertexBuffer, m_pIndexBuffer;
	};
}