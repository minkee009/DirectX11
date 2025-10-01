#pragma once
#include <string>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr

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
}