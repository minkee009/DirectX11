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
		inline const std::vector<VertexType>& GetVertices() const { return m_vertices; }
		inline const std::vector<UINT>& GetIndices() const { return m_indices; }
		Mesh(const std::vector<VertexType>& vertices, const std::vector<UINT>& indices, ID3D11Device* device);
		void Bind(ID3D11DeviceContext* ctx) const;
	private:
		ComPtr<ID3D11Buffer> m_pVertexBuffer, m_pIndexBuffer;
		std::vector<VertexType> m_vertices;
		std::vector<UINT> m_indices;
	};
}