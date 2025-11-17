#pragma once
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include "Singleton.h"

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

namespace MyEngine
{
	class MeshRenderer;
}

namespace MyEngine::D3DCTX
{
	class ShaderManager : public Singleton<ShaderManager>
	{
	private:
		ComPtr<ID3D11VertexShader> m_pDefaultVertexShader;
		ComPtr<ID3D11VertexShader> m_pOutlineVertexShader;
		ComPtr<ID3D11VertexShader> m_pOutlineVertexShader_useRigidBone;
		ComPtr<ID3D11VertexShader> m_pOutlineVertexShader_useSkinningBone;

		ComPtr<ID3D11VertexShader> m_pBlinnPhongVertexShader;
		ComPtr<ID3D11VertexShader> m_pBlinnPhongVertexShader_useRigidBone;
		ComPtr<ID3D11VertexShader> m_pBlinnPhongVertexShader_useSkinningBone;

		ComPtr<ID3D11PixelShader> m_pDefaultPixelShader;
		ComPtr<ID3D11PixelShader> m_pOutlinePixelShader;

		ComPtr<ID3D11PixelShader> m_pBlinnPhongPixelShader;
		ComPtr<ID3D11PixelShader> m_pBlinnPhongToonPixelShader;
		ComPtr<ID3D11PixelShader> m_pBlinnPhongShadowMapPixelShader;

		ComPtr<ID3DBlob> m_pDefaultVSBlob = nullptr;
		ComPtr<ID3DBlob> m_pBlinnPhongVSBlob = nullptr;

		bool CompileLiteralCodeToVertexShader(ID3D11Device* pDevice, ID3D11VertexShader** ppVS, const char* literal, ID3DBlob** ppVSBlob = nullptr);
		bool CompileLiteralCodeToPixelShader(ID3D11Device* pDevice, ID3D11PixelShader** ppPS, const char* literal);



	public:
		void StartUp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
		void ShutDown();

		void BindDefaultShaders(ID3D11DeviceContext* context);
		void BindOutlineShaders(ID3D11DeviceContext* context);
		void BindOutlineShaders(ID3D11DeviceContext* context, MeshRenderer* pMeshRenderer);

		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

		inline ID3D11VertexShader* GetDefaultVertexShader() { return m_pDefaultVertexShader.Get(); }
		inline ID3D11VertexShader* GetOutlineVertexShader() { return m_pOutlineVertexShader.Get(); }
		inline ID3D11VertexShader* GetOutlineVertexShader_RigidBone() { return m_pOutlineVertexShader_useRigidBone.Get(); }
		inline ID3D11VertexShader* GetOutlineVertexShader_SkinningBone() { return m_pOutlineVertexShader_useSkinningBone.Get(); }

		inline ID3D11PixelShader* GetDefaultPixelShader() { return m_pDefaultPixelShader.Get(); }
		inline ID3D11PixelShader* GetOutlinePixelShader() { return m_pOutlinePixelShader.Get(); }

		inline ID3DBlob* GetDefaultVSBlob() { return m_pDefaultVSBlob.Get(); }

		inline ID3D11VertexShader* GetBlinnPhongVertexShader() { return m_pBlinnPhongVertexShader.Get(); }
		inline ID3D11VertexShader* GetBlinnPhongVertexShader_RigidBone() { return m_pBlinnPhongVertexShader_useRigidBone.Get(); }
		inline ID3D11VertexShader* GetBlinnPhongVertexShader_SkinningBone() { return m_pBlinnPhongVertexShader_useSkinningBone.Get(); }

		inline ID3D11PixelShader* GetBlinnPhongPixelShader() { return m_pBlinnPhongPixelShader.Get(); }
		inline ID3D11PixelShader* GetBlinnPhongToonPixelShader() { return m_pBlinnPhongToonPixelShader.Get(); }
		inline ID3D11PixelShader* GetBlinnPhongShadowMapPixelShader() { return m_pBlinnPhongShadowMapPixelShader.Get(); }

		inline ID3DBlob* GetBlinnPhongVSBlob() { return m_pBlinnPhongVSBlob.Get(); }
	};
}