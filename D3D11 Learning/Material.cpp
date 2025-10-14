#include "Material.h"

#include <DirectXTex.h>
#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool MyEngine::Material::InitAndCompileShader(ID3D11Device* device,ShaderType type, const std::wstring& path)
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
		hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, m_vertexShader.GetAddressOf());
		if (FAILED(hr))
		{
			pVSBlob->Release();
			return false;
		}
		m_vsBlob = nullptr;
		m_vsBlob.Attach(pVSBlob); //m_vsBlob가 pVSBlob의 소유권을 갖도록
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
		hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, m_pixelShader.GetAddressOf());
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

bool MyEngine::Material::InitShader(ShaderType type, ID3D11DeviceChild* shader)
{
	switch (type)
	{
	case ShaderType::Vertex:
		m_vertexShader = static_cast<ID3D11VertexShader*>(shader);
		return true;
	case ShaderType::Pixel:
		m_pixelShader = static_cast<ID3D11PixelShader*>(shader);
		return true;
	}
	return false;
}

bool MyEngine::Material::InitTexture(ID3D11DeviceContext* ctx, const std::wstring& name, UINT slot, const std::wstring& path, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
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
	ctx->GetDevice(&device);

	ComPtr<ID3D11ShaderResourceView> pSRV;
	hr = CreateShaderResourceView(device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), pSRV.GetAddressOf());

	if (FAILED(hr))
		return false;
	//----------------------------//


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
	hr = device->CreateSamplerState(&sampDesc, pSampler.GetAddressOf());
	if (FAILED(hr))
		return false;
	//----------------------------//

	textures.push_back(TextureBinding{ std::move(name), slot, pSRV, pSampler });

	return true;
}

void MyEngine::Material::Bind(ID3D11DeviceContext* context)
{
	if (m_vertexShader)
		context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	if (m_pixelShader)
		context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	for (auto& tex : textures)
	{
		if (tex.pSRV)
			context->PSSetShaderResources(tex.slot, 1, tex.pSRV.GetAddressOf());
		if (tex.pSampler)
			context->PSSetSamplers(tex.slot, 1, tex.pSampler.GetAddressOf());
	}
}

ComPtr<ID3D11VertexShader> MyEngine::Material::s_defaultVertexShader = nullptr;
ComPtr<ID3D11PixelShader> MyEngine::Material::s_defaultPixelShader = nullptr;
ComPtr<ID3DBlob> MyEngine::Material::s_defaultVSBlob = nullptr;

void MyEngine::Material::InitDefaultShaders(ID3D11Device* device)
{
	//MVP 정점 셰이더
	if (!s_defaultVertexShader)
	{
		const char* vsCode =
			"struct VS_INPUT                                  \n"
			"{                                               \n"
			"    float3 Pos : POSITION;                      \n"  // float4 -> float3
			"    float3 Norm : NORMAL;                       \n"
			"    float2 Tex : TEXCOORD0;                     \n"
			"};                                              \n"
			"                                                \n"
			"struct PS_INPUT                                 \n"
			"{                                               \n"
			"    float4 Pos : SV_POSITION;                   \n"
			"};                                              \n"
			"                                                \n"
			"cbuffer ConstantBuffer                          \n"
			"{                                               \n"
			"    matrix mWorld;                              \n"
			"    matrix mView;                               \n"
			"    matrix mProjection;                         \n"
			"};                                              \n"
			"                                                \n"
			"PS_INPUT VS(VS_INPUT input)                     \n"
			"{                                               \n"
			"    PS_INPUT output = (PS_INPUT)0;              \n"
			"                                                \n"
			"    // 변환                                     \n"
			"    float4 worldPos = mul(float4(input.Pos, 1.0f), mWorld);  \n"
			"    float4 viewPos = mul(worldPos, mView);                   \n"
			"    output.Pos = mul(viewPos, mProjection);                  \n"
			"                                                \n"
			"    return output;                              \n"
			"}                                               \n";

		ID3DBlob* pVSBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "VS", "vs_4_0",
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
			return;
		}

		if (pErrorBlob)
			pErrorBlob->Release();

		hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
			nullptr, s_defaultVertexShader.GetAddressOf());

		if (FAILED(hr))
		{
			pVSBlob->Release();
			MessageBox(nullptr, L"버텍스 셰이더 생성 실패", L"오류", MB_OK);
			return;
		}

		s_defaultVSBlob = nullptr;
		s_defaultVSBlob.Attach(pVSBlob);
	}

	//단일 픽셀 셰이더
	if (!s_defaultPixelShader)
	{
		const char* psCode =
			"struct PS_INPUT                                  \n"
			"{                                               \n"
			"    float4 Pos : SV_POSITION;                   \n"
			"};                                              \n"
			"                                                \n"
			"float4 PS(PS_INPUT input) : SV_Target           \n"
			"{                                               \n"
			"    return float4(1.0f, 0.0f, 1.0f, 1.0f);      \n"
			"}                                               \n";

		ID3DBlob* pPSBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		HRESULT hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "PS", "ps_4_0",
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
				L"기본 픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
			return;
		}

		if (pErrorBlob)
			pErrorBlob->Release();

		hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
			nullptr, s_defaultPixelShader.GetAddressOf());
		pPSBlob->Release();

		if (FAILED(hr))
		{
			MessageBox(nullptr, L"픽셀 셰이더 생성 실패", L"오류", MB_OK);
			return;
		}
	}
}

void MyEngine::Material::ReleaseDefaultShaders()
{
	s_defaultVertexShader = nullptr;
	s_defaultPixelShader = nullptr;
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
