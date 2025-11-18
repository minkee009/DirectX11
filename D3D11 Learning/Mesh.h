#pragma once
#include "InputLayoutManager.h"
#include <vector>
#include <assimp/mesh.h>
#include <string>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <d3d11.h>
#include <directxtk/SimpleMath.h>

#include "Resource.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace MyEngine
{
	class Mesh : public Resource
	{
	private:
		ComPtr<ID3D11Buffer> m_pVertexBuffer, m_pIndexBuffer;
		std::vector<DefaultVertex> m_vertices;
		std::vector<UINT> m_indices;
		BoundingBox m_bbox;

	public:
		inline std::vector<DefaultVertex>& GetVertices() { return m_vertices; }
		inline std::vector<UINT>& GetIndices() { return m_indices; }
		inline const BoundingBox& GetBBox() const { return m_bbox; }

		void CreateBuffers(ID3D11Device* pDevice);

		Mesh(const std::vector<DefaultVertex>& vertices, const std::vector<UINT>& indices, const BoundingBox& bbox);
		~Mesh();
		void Bind(ID3D11DeviceContext* ctx);
	};
}