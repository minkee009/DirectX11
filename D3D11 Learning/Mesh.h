#pragma once
#include <string>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <unordered_map>
#include <algorithm>

#include "Transform.h"


using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyEngine
{
	enum class VertexType
	{
		Pos = 0,
		PosNormUV,
	};

	struct MeshVertex
	{
		XMFLOAT3 pos;
		XMFLOAT3 nor;
		XMFLOAT2 uv;
	};

	struct SubMesh
	{
		std::vector<MeshVertex> vertices;
		std::vector<UINT> indices;

		ComPtr<ID3D11Buffer> pVertexBuffer;
		ComPtr<ID3D11Buffer> pIndexBuffer;

		std::wstring materialName; // usemtlø° ¥Î¿¿
	};

	class Mesh
	{
	private:
		VertexType m_vertexType = VertexType::PosNormUV;
		std::vector<SubMesh> m_subMeshes;

		bool LoadFromFile(ID3D11Device* device, std::wstring path);
		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	public:
		Mesh();
		~Mesh();
		static std::unique_ptr<Mesh> CreateFromFile(ID3D11Device* device, std::wstring path);

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
		inline VertexType GetVertexType() const { return m_vertexType; }
	};

	struct MeshVertexHash
	{
		size_t operator()(const MeshVertex& v) const
		{
			size_t h1 = std::hash<float>{}(v.pos.x) ^ (std::hash<float>{}(v.pos.y) << 1) ^ (std::hash<float>{}(v.pos.z) << 2);
			size_t h2 = std::hash<float>{}(v.nor.x) ^ (std::hash<float>{}(v.nor.y) << 1) ^ (std::hash<float>{}(v.nor.z) << 2);
			size_t h3 = std::hash<float>{}(v.uv.x) ^ (std::hash<float>{}(v.uv.y) << 1);

			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	struct MeshVertexEqual
	{
		bool operator()(const MeshVertex& v1, const MeshVertex& v2) const
		{
			return v1.pos.x == v2.pos.x && v1.pos.y == v2.pos.y && v1.pos.z == v2.pos.z &&
				v1.nor.x == v2.nor.x && v1.nor.y == v2.nor.y && v1.nor.z == v2.nor.z &&
				v1.uv.x == v2.uv.x && v1.uv.y == v2.uv.y;
		}
	};
}