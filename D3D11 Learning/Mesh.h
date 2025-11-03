#pragma once
#include "VertexType.h"
#include <vector>
#include <assimp/mesh.h>
#include <string>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <d3d11.h>
#include "AABB.h"

using namespace Microsoft::WRL;

namespace MyEngine
{
	class Mesh
	{
	private:
		ComPtr<ID3D11Buffer> m_pVertexBuffer, m_pIndexBuffer;
		std::vector<VertexType> m_vertices;
		std::vector<UINT> m_indices;
		AABB m_aabb;
	public:
		inline std::vector<VertexType>& GetVertices() { return m_vertices; }
		inline std::vector<UINT>& GetIndices() { return m_indices; }
		inline const AABB& GetAABB() const { return m_aabb; }
		Mesh(const std::vector<VertexType>& vertices, const std::vector<UINT>& indices, const AABB& aabb);
		void Bind(ID3D11DeviceContext* ctx);
	};
}