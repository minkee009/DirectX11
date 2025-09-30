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

		std::wstring materialName; // usemtl에 대응
	};

	struct MaterialRef
	{
		std::wstring name;
		// 실제 머티리얼 데이터 (diffuse texture, normal map 등)
		// 추후 로드 단계에서 연결
	};

	class Mesh
	{
	private:
		std::vector<SubMesh> m_subMeshes;

		//=== 임시 머터리얼 코드 ===//
		ComPtr<ID3D11InputLayout> m_inputLayout = nullptr;  //추후에 머터리얼 클래스에서 관리하는걸로 변경
		ComPtr<ID3D11VertexShader> m_pVertexShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pPixelShader = nullptr;
		//=========================//

		bool LoadFromFile(ID3D11Device* device, std::wstring path);
		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	public:
		Mesh();
		~Mesh();
		static std::unique_ptr<Mesh> CreateFromFile(ID3D11Device* device, std::wstring path);

		void Draw(ID3D11DeviceContext* ctx);
	};
}