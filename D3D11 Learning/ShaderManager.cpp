#include "ShaderManager.h"
#include "LiteralShaderCode.h"
#include "StaticMeshRenderer.h"
#include "RigidMeshRenderer.h"
#include "SkinningMeshRenderer.h"

#include <DirectXTex.h>
#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool MyEngine::D3DCTX::ShaderManager::CompileLiteralCodeToVertexShader(ID3D11Device* pDevice, ID3D11VertexShader** ppVS, const char* literal, ID3DBlob** ppVSBlob)
{
	if (*ppVS)
		return true;

	ID3DBlob* pVSBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	HRESULT hr = D3DCompile(literal, strlen(literal), nullptr, nullptr, nullptr, "VS", "vs_4_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVSBlob, &pErrorBlob);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			const char* errorMsg = reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer());
			OutputDebugStringA("버텍스 셰이더 컴파일 오류:\n");
			OutputDebugStringA(errorMsg);
			pErrorBlob->Release();
		}
		MessageBox(nullptr,
			L"기본 버텍스 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
		return false;
	}

	if (pErrorBlob)
		pErrorBlob->Release();

	hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
		nullptr, ppVS);

	if (FAILED(hr))
	{
		pVSBlob->Release();
		MessageBox(nullptr, L"버텍스 셰이더 생성 실패", L"오류", MB_OK);
		return false;
	}

	if (ppVSBlob)
	{
		if (*ppVSBlob)
			(*ppVSBlob)->Release();

		*ppVSBlob = pVSBlob;
	}
	else {
		pVSBlob->Release();
	}

	return true;
}

bool MyEngine::D3DCTX::ShaderManager::CompileLiteralCodeToPixelShader(ID3D11Device* pDevice, ID3D11PixelShader** ppPS, const char* literal)
{
	if (*ppPS)
		return true;

	ID3DBlob* pPSBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	HRESULT hr = D3DCompile(literal, strlen(literal), nullptr, nullptr, nullptr, "PS", "ps_4_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pPSBlob, &pErrorBlob);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			const char* errorMsg = reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer());
			OutputDebugStringA("픽셀 셰이더 컴파일 오류:\n");
			OutputDebugStringA(errorMsg);
			pErrorBlob->Release();
		}
		MessageBox(nullptr,
			L"블린 퐁 픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
		return false;
	}

	if (pErrorBlob)
		pErrorBlob->Release();

	hr = pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
		nullptr, ppPS);
	pPSBlob->Release();

	if (FAILED(hr))
	{
		MessageBox(nullptr, L"픽셀 셰이더 생성 실패", L"오류", MB_OK);
		return false;
	}

	return true;
}

void MyEngine::D3DCTX::ShaderManager::StartUp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CompileLiteralCodeToVertexShader(pDevice, m_pDefaultVertexShader.GetAddressOf(), g_vscode_def, m_pDefaultVSBlob.GetAddressOf());
	CompileLiteralCodeToVertexShader(pDevice, m_pOutlineVertexShader.GetAddressOf(), g_vscode_outline_static);	
	CompileLiteralCodeToVertexShader(pDevice, m_pOutlineVertexShader_useRigidBone.GetAddressOf(), g_vscode_outline_rigid);
	CompileLiteralCodeToVertexShader(pDevice, m_pOutlineVertexShader_useSkinningBone.GetAddressOf(), g_vscode_outline_skinning);


	CompileLiteralCodeToPixelShader(pDevice, m_pDefaultPixelShader.GetAddressOf(), g_pscode_def);
	CompileLiteralCodeToPixelShader(pDevice, m_pOutlinePixelShader.GetAddressOf(), g_pscode_outline);

	////블린 퐁 정점 셰이더
	//CompileLiteralCodeToVertexShader(pDevice, m_pCommonVertexShader.GetAddressOf(), g_vscode_common_static, m_pBlinnPhongVSBlob.GetAddressOf());
	//CompileLiteralCodeToVertexShader(pDevice, m_pCommonVertexShader_useRigidBone.GetAddressOf(), g_vscode_common_rigid);
	//CompileLiteralCodeToVertexShader(pDevice, m_pCommonVertexShader_useSkinningBone.GetAddressOf(), g_vscode_common_skinning);

	////블린 퐁 픽셀 셰이더
	CompileLiteralCodeToPixelShader(pDevice, m_pBlinnPhongPixelShader.GetAddressOf(), g_pscode_blinnphong);
	CompileLiteralCodeToPixelShader(pDevice, m_pBlinnPhongToonPixelShader.GetAddressOf(), g_pscode_blinnphong_toon);
	CompileLiteralCodeToPixelShader(pDevice, m_pBlinnPhongShadowMapPixelShader.GetAddressOf(), g_pscode_blinnphong_shadowmap);

	//디퍼드 용 정점 셰이더
	CompileLiteralCodeToVertexShader(pDevice, m_pCommonVertexShader.GetAddressOf(), g_vscode_deffered_static, m_pBlinnPhongVSBlob.GetAddressOf());
	CompileLiteralCodeToVertexShader(pDevice, m_pCommonVertexShader_useRigidBone.GetAddressOf(), g_vscode_common_rigid);
	CompileLiteralCodeToVertexShader(pDevice, m_pCommonVertexShader_useSkinningBone.GetAddressOf(), g_vscode_deffered_skinning);

	CompileLiteralCodeToVertexShader(pDevice, m_pShadowCastVertexShader.GetAddressOf(), g_vscode_shadowcast_common);

	//BRDF 픽셀 셰이더
	CompileLiteralCodeToPixelShader(pDevice, m_pBRDFPixelShader.GetAddressOf(), g_pscode_BRDF_cook_torrance);

	CompileLiteralCodeToVertexShader(pDevice, m_pPostProcessingVertexShader.GetAddressOf(), g_postprocess_vscode_quad);
	CompileLiteralCodeToPixelShader(pDevice, m_pPostProcessingPixelShader.GetAddressOf(), g_postprocess_pscode_ACES_toneMapping);

	CompileLiteralCodeToPixelShader(pDevice, m_pDefferedGeometryPixelShader.GetAddressOf(), g_pscode_deffered_Geometry);
	CompileLiteralCodeToPixelShader(pDevice, m_pDefferedLightPixelShader.GetAddressOf(), g_pscode_deffered_Light);
	CompileLiteralCodeToPixelShader(pDevice, m_pDefferedAdditivePointLightPixelShader.GetAddressOf(), g_pscode_deffered_AdditivePointLight);
}

void MyEngine::D3DCTX::ShaderManager::ShutDown()
{
	m_pShadowCastVertexShader = nullptr;
	m_pDefaultVertexShader = nullptr;
	m_pDefaultPixelShader = nullptr;
	m_pOutlineVertexShader = nullptr;
	m_pOutlineVertexShader_useRigidBone = nullptr;
	m_pOutlineVertexShader_useSkinningBone = nullptr;
	m_pOutlinePixelShader = nullptr;
	m_pDefaultVSBlob = nullptr;

	m_pCommonVertexShader = nullptr;
	m_pCommonVertexShader_useRigidBone = nullptr;
	m_pCommonVertexShader_useSkinningBone = nullptr;
	m_pBlinnPhongPixelShader = nullptr;
	m_pBlinnPhongToonPixelShader = nullptr;
	m_pBlinnPhongShadowMapPixelShader = nullptr;
	m_pBRDFPixelShader = nullptr;
	m_pBlinnPhongVSBlob = nullptr;

	m_pPostProcessingVertexShader = nullptr;
	m_pPostProcessingPixelShader = nullptr;

	m_pDefferedGeometryPixelShader = nullptr;
	m_pDefferedLightPixelShader = nullptr;
	m_pDefferedAdditivePointLightPixelShader = nullptr;
}

void MyEngine::D3DCTX::ShaderManager::BindDefaultShaders(ID3D11DeviceContext* context)
{
	//context->VSSetShader(Material::GetDefaultVertexShader(), nullptr, 0);
	context->PSSetShader(GetDefaultPixelShader(), nullptr, 0);
}

void MyEngine::D3DCTX::ShaderManager::BindOutlineShaders(ID3D11DeviceContext* context)
{
	context->VSSetShader(GetOutlineVertexShader(), nullptr, 0);
	context->PSSetShader(GetOutlinePixelShader(), nullptr, 0);
}

void MyEngine::D3DCTX::ShaderManager::BindOutlineShaders(ID3D11DeviceContext* context, MeshRenderer* pMeshRenderer)
{
	if (StaticMeshRenderer* pStatic = dynamic_cast<StaticMeshRenderer*>(pMeshRenderer))
	{
		context->VSSetShader(GetOutlineVertexShader(), nullptr, 0);
	}
	else if (RigidMeshRenderer* pStatic = dynamic_cast<RigidMeshRenderer*>(pMeshRenderer))
	{
		context->VSSetShader(GetOutlineVertexShader_RigidBone(), nullptr, 0);
	}
	else if (SkinningMeshRenderer* pStatic = dynamic_cast<SkinningMeshRenderer*>(pMeshRenderer))
	{
		context->VSSetShader(GetOutlineVertexShader_SkinningBone(), nullptr, 0);
	}
	else
	{
		BindOutlineShaders(context);
		return;
	}

	context->PSSetShader(GetOutlinePixelShader(), nullptr, 0);
}

HRESULT MyEngine::D3DCTX::ShaderManager::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			auto cr = reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer());

			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}
