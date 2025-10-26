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
		inline std::vector<VertexType>& GetVertices() { return m_vertices; }
		inline std::vector<UINT>& GetIndices() { return m_indices; }
		Mesh(const std::vector<VertexType>& vertices, const std::vector<UINT>& indices);
		void Bind(ID3D11DeviceContext* ctx);
	private:
		ComPtr<ID3D11Buffer> m_pVertexBuffer, m_pIndexBuffer;
		std::vector<VertexType> m_vertices;
		std::vector<UINT> m_indices;
	};
}