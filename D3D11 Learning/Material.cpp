#include "Material.h"
#include "MaterialShaderCode.h"

#include <DirectXTex.h>
#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool MyEngine::Material::InitAndCompileShader(ID3D11Device* device, ShaderType type, const std::wstring& path)
{
	switch (type)
	{
	case ShaderType::Vertex:
	{
		ID3DBlob* pVSBlob = nullptr;
		HRESULT hr = CompileShaderFromFile(path.c_str(), "VS", "vs_4_0", &pVSBlob);
		if (FAILED(hr))
		{
			MessageBox(nullptr,
				L"버텍스 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
			return false;
		}
		hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf());
		if (FAILED(hr))
		{
			pVSBlob->Release();
			return false;
		}
		m_pVSBlob = nullptr;
		m_pVSBlob.Attach(pVSBlob); //m_vsBlob가 pVSBlob의 소유권을 갖도록
		return true;
	}
	case ShaderType::Pixel:
	{
		ID3DBlob* pPSBlob = nullptr;
		HRESULT hr = CompileShaderFromFile(path.c_str(), "PS", "ps_4_0", &pPSBlob);
		if (FAILED(hr))
		{
			MessageBox(nullptr,
				L"픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
			return false;
		}
		hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf());
		if (FAILED(hr))
		{
			pPSBlob->Release();
			return false;
		}
		pPSBlob->Release();
		return true;
	}
	}
	return false;
}

bool MyEngine::Material::InitShader(ShaderType type, ID3D11DeviceChild* shader, ID3DBlob* vsBlob)
{
	switch (type)
	{
	case ShaderType::Vertex:
		m_pVertexShader = static_cast<ID3D11VertexShader*>(shader);
		m_pVSBlob = vsBlob;
		return true;
	case ShaderType::Pixel:
		m_pPixelShader = static_cast<ID3D11PixelShader*>(shader);
		return true;
	}

	return false;
}

bool MyEngine::Material::InitSampler(ID3D11Device* device,D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
{

	//-------샘플러 상태 생성------//
	ComPtr<ID3D11SamplerState> pSampler;
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = filter;
	sampDesc.AddressU = addressMode;
	sampDesc.AddressV = addressMode;
	sampDesc.AddressW = addressMode;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = device->CreateSamplerState(&sampDesc, pSampler.GetAddressOf());
	if (FAILED(hr))
		return false;
	//----------------------------//

	m_pSampler = pSampler;

	return true;
}

bool MyEngine::Material::InitSampler(ID3D11SamplerState* pSampler)
{
	if (!pSampler)
	{
		m_pSampler = nullptr;
		return false;
	}

	m_pSampler = pSampler;
	return true;
}

bool MyEngine::Material::InitTexture(const std::string& name, TextureType type, UINT slot, ID3D11ShaderResourceView* textureView)
{
	if(!textureView)
		return false;

	m_textureFlags |= static_cast<UINT>(type);
	m_textures.push_back(TextureBinding{ type, name, slot, textureView });

	return true;
}

void MyEngine::Material::CreateConstantBuffer(ID3D11DeviceContext* context)
{
	if (m_materialCB)
		return;

	//상수 버퍼 생성
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.ByteWidth = sizeof(MaterialCB);
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;

	ID3D11Device* pDevice;
	context->GetDevice(&pDevice);

	HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_materialCB.GetAddressOf());
	pDevice->Release();
	if (FAILED(hr))
		return;

	MaterialCB cb;
	cb.textureFlags = m_textureFlags;
	cb.baseColor = m_baseColor;
	context->UpdateSubresource(m_materialCB.Get(), 0, nullptr, &cb, 0, 0);
}

bool MyEngine::Material::InitAndConvertTexture(ID3D11DeviceContext* context, TextureType type, const std::string& name, UINT slot, const std::wstring& path)
{
	//dds인지 아닌지 확인
	bool isDDS = false;

	size_t extPos = path.rfind(L'.');
	if (extPos != std::wstring::npos)
	{
		std::wstring ext = path.substr(extPos);
		if (ext == L".dds" || ext == L".DDS")
			isDDS = true;
	}

	//-------- 텍스쳐 로드 --------//
	HRESULT hr = S_OK;

	DirectX::ScratchImage image;
	if (isDDS)
	{
		hr = LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, image);
	}
	else
	{
		hr = LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, nullptr, image);
	}

	if (FAILED(hr))
		return false;

	ID3D11Device* device = nullptr;
	context->GetDevice(&device);

	ComPtr<ID3D11ShaderResourceView> pSRV;
	hr = CreateShaderResourceView(device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), pSRV.GetAddressOf());
	device->Release();
	if (FAILED(hr))
		return false;

	m_textureFlags |= static_cast<UINT>(type);
	m_textures.push_back(TextureBinding{ type, name, slot, pSRV });
	return true;
}

bool MyEngine::Material::InitAndConvertTextureFromMemory(ID3D11DeviceContext* context, TextureType type, const std::string& name, UINT slot, const uint8_t* pData, size_t dataSize, const std::wstring& formatExt)
{
	if (!pData || dataSize == 0)
		return false;

	HRESULT hr = S_OK;
	DirectX::ScratchImage image;

	if (formatExt == L".dds" || formatExt == L".DDS")
	{
		// DDS 포맷은 LoadFromDDSMemory 사용
		hr = DirectX::LoadFromDDSMemory(pData, dataSize, DirectX::DDS_FLAGS_NONE, nullptr, image);
	}
	else // 대부분의 WIC 포맷 (png, jpg, bmp, tiff 등)
	{
		// WIC 포맷은 LoadFromWICMemory 사용
		hr = DirectX::LoadFromWICMemory(pData, dataSize, DirectX::WIC_FLAGS_NONE, nullptr, image);
	}

	if (FAILED(hr))
	{
		// 로딩 실패
		return false;
	}

	// D3D11 Device 가져오기
	ComPtr<ID3D11Device> device;
	context->GetDevice(device.GetAddressOf());

	// ShaderResourceView 생성
	ComPtr<ID3D11ShaderResourceView> pSRV;
	hr = DirectX::CreateShaderResourceView(
		device.Get(),
		image.GetImages(),
		image.GetImageCount(),
		image.GetMetadata(),
		pSRV.GetAddressOf());

	device->Release();
	if (FAILED(hr))
		return false;

	// 엔진 구조에 저장
	m_textureFlags |= static_cast<UINT>(type);
	m_textures.push_back(TextureBinding{ type, name, slot, pSRV });

	return true;
}

MyEngine::Material::Material(const std::string& name)
	: m_name(name)
{
}

MyEngine::Material::~Material()
{
	m_materialCB = nullptr;
	m_pVertexShader = nullptr;
	m_pPixelShader = nullptr;
	m_pVSBlob = nullptr;
	m_pSampler = nullptr;
	m_textures.clear();
}

void MyEngine::Material::Bind(ID3D11DeviceContext* context)
{
	//상수버퍼 설정
	context->PSSetConstantBuffers(1, 1, m_materialCB.GetAddressOf());

	if (m_pVertexShader)
		context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	if (m_pPixelShader)
		context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	for (auto& tex : m_textures)
	{
		if (tex.pSRV)
			context->PSSetShaderResources(tex.slot, 1, tex.pSRV.GetAddressOf());
		if (m_pSampler)
			context->PSSetSamplers(tex.slot, 1, m_pSampler.GetAddressOf());
	}

	//if (m_textures.empty())
	//{
	//	BindDefaultShaders(context);
	//}
}

ComPtr<ID3D11VertexShader> MyEngine::Material::s_pDefaultVertexShader = nullptr;
ComPtr<ID3D11VertexShader> MyEngine::Material::s_pOutlineVertexShader = nullptr;
ComPtr<ID3D11VertexShader> MyEngine::Material::s_pOutlineVertexShader_useRigidBone = nullptr;
ComPtr<ID3D11VertexShader> MyEngine::Material::s_pOutlineVertexShader_useSkinningBone = nullptr;
ComPtr<ID3D11PixelShader> MyEngine::Material::s_pDefaultPixelShader = nullptr;
ComPtr<ID3D11PixelShader> MyEngine::Material::s_pOutlinePixelShader = nullptr;
ComPtr<ID3DBlob> MyEngine::Material::s_pDefaultVSBlob = nullptr;

ComPtr<ID3D11VertexShader> MyEngine::Material::s_pBlinnPhongVertexShader = nullptr;
ComPtr<ID3D11VertexShader> MyEngine::Material::s_pBlinnPhongVertexShader_useRigidBone = nullptr;
ComPtr<ID3D11VertexShader> MyEngine::Material::s_pBlinnPhongVertexShader_useSkinningBone = nullptr;
ComPtr<ID3D11PixelShader> MyEngine::Material::s_pBlinnPhongPixelShader = nullptr;
ComPtr<ID3D11PixelShader> MyEngine::Material::s_pBlinnPhongToonPixelShader = nullptr;
ComPtr<ID3D11PixelShader> MyEngine::Material::s_pBlinnPhongShadowMapPixelShader = nullptr;
ComPtr<ID3DBlob> MyEngine::Material::s_pBlinnPhongVSBlob = nullptr;

bool MyEngine::Material::CompileLiteralCodeToVertexShader(ID3D11Device* pDevice, ID3D11VertexShader** ppVS, const char* literal, ID3DBlob** ppVSBlob)
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

bool MyEngine::Material::CompileLiteralCodeToPixelShader(ID3D11Device* pDevice, ID3D11PixelShader** ppPS, const char* literal)
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

void MyEngine::Material::InitDefaultShaders(ID3D11Device* device)
{
	//MVP 정점 셰이더
	CompileLiteralCodeToVertexShader(device, s_pDefaultVertexShader.GetAddressOf(), g_vscode_def, s_pDefaultVSBlob.GetAddressOf());
	CompileLiteralCodeToVertexShader(device, s_pOutlineVertexShader.GetAddressOf(), g_vscode_outline);
	CompileLiteralCodeToVertexShader(device, s_pOutlineVertexShader_useSkinningBone.GetAddressOf(), g_vscode_outline_useSkinning);

	CompileLiteralCodeToPixelShader(device, s_pDefaultPixelShader.GetAddressOf(), g_pscode_def);
	CompileLiteralCodeToPixelShader(device, s_pOutlinePixelShader.GetAddressOf(), g_pscode_outline);
}

void MyEngine::Material::ReleaseDefaultShaders()
{
	s_pDefaultVertexShader = nullptr;
	s_pDefaultPixelShader = nullptr;
	s_pOutlineVertexShader = nullptr;
	s_pOutlineVertexShader_useRigidBone = nullptr;
	s_pOutlineVertexShader_useSkinningBone = nullptr;
	s_pOutlinePixelShader = nullptr;
	s_pDefaultVSBlob = nullptr;
}

void MyEngine::Material::BindDefaultShaders(ID3D11DeviceContext* context)
{
	context->VSSetShader(Material::GetDefaultVertexShader(), nullptr, 0);
	context->PSSetShader(Material::GetDefaultPixelShader(), nullptr, 0);
}

void MyEngine::Material::BindOutlineShaders(ID3D11DeviceContext* context)
{
	context->VSSetShader(Material::GetOutlineVertexShader(), nullptr, 0);
	context->PSSetShader(Material::GetOutlinePixelShader(), nullptr, 0);
}

void MyEngine::Material::InitBlinnPhongShaders(ID3D11Device* device)
{
    //블린 퐁 정점 셰이더
	CompileLiteralCodeToVertexShader(device, s_pBlinnPhongVertexShader.GetAddressOf(), g_vscode_blinnphong, s_pBlinnPhongVSBlob.GetAddressOf());
	CompileLiteralCodeToVertexShader(device, s_pBlinnPhongVertexShader_useRigidBone.GetAddressOf(), g_vscode_blinnphong_rigid);
	CompileLiteralCodeToVertexShader(device, s_pBlinnPhongVertexShader_useSkinningBone.GetAddressOf(), g_vscode_blinnphong_skinning);

	//블린 퐁 픽셀 셰이더
    CompileLiteralCodeToPixelShader(device, s_pBlinnPhongPixelShader.GetAddressOf(), g_pscode_blinnphong);
    CompileLiteralCodeToPixelShader(device, s_pBlinnPhongToonPixelShader.GetAddressOf(), g_pscode_blinnphong_toon);
    CompileLiteralCodeToPixelShader(device, s_pBlinnPhongShadowMapPixelShader.GetAddressOf(), g_pscode_blinnphong_shadowmap);
}

void MyEngine::Material::ReleaseBlinnPhongShaders()
{
	s_pBlinnPhongVertexShader = nullptr;
	s_pBlinnPhongVertexShader_useRigidBone = nullptr;
	s_pBlinnPhongVertexShader_useSkinningBone = nullptr;
	s_pBlinnPhongPixelShader = nullptr;
	s_pBlinnPhongToonPixelShader = nullptr;
	s_pBlinnPhongShadowMapPixelShader = nullptr;
	s_pBlinnPhongVSBlob = nullptr;
}

HRESULT MyEngine::Material::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
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
