#include "Material.h"

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

void MyEngine::Material::Bind(ID3D11DeviceContext* context)
{
	//상수버퍼 업데이트
	if (!m_materialCB)
	{
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
		if (FAILED(hr))
			return;
	}

	MaterialCB cb;
	cb.textureFlags = m_textureFlags;
	cb.baseColor = m_baseColor;
	context->UpdateSubresource(m_materialCB.Get(), 0, nullptr, &cb, 0, 0);
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
ComPtr<ID3D11PixelShader> MyEngine::Material::s_pDefaultPixelShader = nullptr;
ComPtr<ID3DBlob> MyEngine::Material::s_pDefaultVSBlob = nullptr;

ComPtr<ID3D11VertexShader> MyEngine::Material::s_pBlinnPhongVertexShader = nullptr;
ComPtr<ID3D11VertexShader> MyEngine::Material::s_pBlinnPhongVertexShader_useRigidBone = nullptr;
ComPtr<ID3D11VertexShader> MyEngine::Material::s_pBlinnPhongVertexShader_useSkinningBone = nullptr;
ComPtr<ID3D11PixelShader> MyEngine::Material::s_pBlinnPhongPixelShader = nullptr;
ComPtr<ID3DBlob> MyEngine::Material::s_pBlinnPhongVSBlob = nullptr;

void MyEngine::Material::InitDefaultShaders(ID3D11Device* device)
{
	//MVP 정점 셰이더
	if (!s_pDefaultVertexShader)
	{
		const char* vsCode = R"(
struct VS_INPUT                                   
{                                                 
	float4 Pos : POSITION;       // float4 -> float3
	float3 Norm : NORMAL;                         
	float3 Tan : TANGENT;                         
	float2 Tex : TEXCOORD0;               
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;        
};                                                
			                                                   
struct PS_INPUT                                   
{                                                 
	float4 Pos : SV_POSITION;                     
};                                                
			                                                   
cbuffer ConstantBuffer                            
{                                                 
	matrix mWorld;                                
	matrix mView;                                 
	matrix mProjection;                           
};                                                
			                                                   
PS_INPUT VS(VS_INPUT input)                       
{                                                 
	PS_INPUT output = (PS_INPUT)0;                
			                                                   
	// 변환                                        
	float4 worldPos = mul(input.Pos, mWorld);     
	float4 viewPos = mul(worldPos, mView);                     
	output.Pos = mul(viewPos, mProjection);                    
			                                                   
	return output;                                
}
)";

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
			nullptr, s_pDefaultVertexShader.GetAddressOf());

		if (FAILED(hr))
		{
			pVSBlob->Release();
			MessageBox(nullptr, L"버텍스 셰이더 생성 실패", L"오류", MB_OK);
			return;
		}

		s_pDefaultVSBlob = nullptr;
		s_pDefaultVSBlob.Attach(pVSBlob);
	}

	//단일 픽셀 셰이더
	if (!s_pDefaultPixelShader)
	{
		const char* psCode = R"(			 
struct PS_INPUT                                    
{                                                 
	float4 Pos : SV_POSITION;                     
};                                                
			                                                   
float4 PS(PS_INPUT input) : SV_Target             
{                                                 
	return float4(1.0f, 0.0f, 1.0f, 1.0f);        
}  
)";

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
			nullptr, s_pDefaultPixelShader.GetAddressOf());
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
	s_pDefaultVertexShader = nullptr;
	s_pDefaultPixelShader = nullptr;
	s_pDefaultVSBlob = nullptr;
}

void MyEngine::Material::BindDefaultShaders(ID3D11DeviceContext* context)
{
	context->VSSetShader(Material::GetDefaultVertexShader(), nullptr, 0);
	context->PSSetShader(Material::GetDefaultPixelShader(), nullptr, 0);
}

void MyEngine::Material::InitBlinnPhongShaders(ID3D11Device* device)
{
	//블린 퐁 정점 셰이더
	if (!s_pBlinnPhongVertexShader)
	{
		const char* vsCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vOutputColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
	matrix LightViewProjection;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    output.Pos = mul(input.Pos, World);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
	// finalWorld 행렬이 적용된 위치를 LightViewProjection으로 변환
	output.LightPos = mul(float4(output.WorldPos, 1.0f),LightViewProjection);

    output.Norm = normalize(mul(input.Norm, (float3x3) World));
    output.Tan = normalize(mul(input.Tan, (float3x3) World));
    output.Tex = input.Tex;
    
    return output;
}
)";

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
				L"블린 퐁 버텍스 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
			return;
		}

		if (pErrorBlob)
			pErrorBlob->Release();

		hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
			nullptr, s_pBlinnPhongVertexShader.GetAddressOf());

		if (FAILED(hr))
		{
			pVSBlob->Release();
			MessageBox(nullptr, L"버텍스 셰이더 생성 실패", L"오류", MB_OK);
			return;
		}

		s_pBlinnPhongVSBlob = nullptr;
		s_pBlinnPhongVSBlob.Attach(pVSBlob);
	}

	if (!s_pBlinnPhongVertexShader_useRigidBone)
	{
		const char* vsCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

cbuffer BoneBuffer : register(b2)
{
	matrix ModelMatricies[128];
}
cbuffer BoneBuffer : register(b3)
{
	uint boneIdx;
}


struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
	matrix tWorld = mul(ModelMatricies[boneIdx], World);

    output.Pos = mul(input.Pos, tWorld);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) tWorld));
    output.Tan = normalize(mul(input.Tan, (float3x3) tWorld));
    output.Tex = input.Tex;
    
    return output;
}
)";

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
				L"블린 퐁 버텍스 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
			return;
		}

		if (pErrorBlob)
			pErrorBlob->Release();

		hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
			nullptr, s_pBlinnPhongVertexShader_useRigidBone.GetAddressOf());

		if (FAILED(hr))
		{
			pVSBlob->Release();
			MessageBox(nullptr, L"버텍스 셰이더 생성 실패", L"오류", MB_OK);
			return;
		}
	}

	if (!s_pBlinnPhongVertexShader_useSkinningBone)
	{
		const char* vsCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vOutputColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
	matrix LightViewProjection;
}

cbuffer BoneModelBuffer : register(b2)
{
	matrix ModelMatricies[128];
}

cbuffer BoneOffsetBuffer : register(b3)
{
	matrix OffsetMatricies[128];
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
	
    matrix skinningMatrix =  {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };

    for (int i = 0; i < 4; i++)
    {
        skinningMatrix += mul(OffsetMatricies[input.BoneIndices[i]],ModelMatricies[input.BoneIndices[i]]) * input.BoneWeights[i] ;
    }

	matrix finalWorld = mul(skinningMatrix,World);

    output.Pos = mul(input.Pos, finalWorld);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

	// finalWorld 행렬이 적용된 위치를 LightViewProjection으로 변환
	output.LightPos = mul(float4(output.WorldPos, 1.0f),LightViewProjection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) finalWorld));
    output.Tan = normalize(mul(input.Tan, (float3x3) finalWorld));
    output.Tex = input.Tex;
    
    return output;
}
)";

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
				L"블린 퐁 버텍스 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
			return;
		}

		if (pErrorBlob)
			pErrorBlob->Release();

		hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
			nullptr, s_pBlinnPhongVertexShader_useSkinningBone.GetAddressOf());

		if (FAILED(hr))
		{
			pVSBlob->Release();
			MessageBox(nullptr, L"버텍스 셰이더 생성 실패", L"오류", MB_OK);
			return;
		}
	}


	//블린 퐁 픽셀 셰이더
	if (!s_pBlinnPhongPixelShader)
	{
		const char* psCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vOutputColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
	matrix LightViewProjection;
}

cbuffer MaterialBuffer : register(b1)
{
	uint textureFlags; 
	float4 baseColor;
}

Texture2D txDiffuse : register(t0);
TextureCube skyBoxTX : register(t1);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
Texture2D emmisiveMap : register(t4);
Texture2D lutMap : register(t5);
SamplerState samLinear : register(s0);
Texture2D shadowMap : register(t6); 
SamplerComparisonState samShadow : register(s1); // 하드웨어 비교를 위한 샘플러


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4; 
	uint  IsFrontFace : SV_IsFrontFace; 
};

// 그림자 테스트 함수 (0.0: 그림자, 1.0: 밝음)
float CalculateShadow(float4 LightPos)
{
    float3 projCoords = LightPos.xyz / LightPos.w;
    float2 texCoords; 
    texCoords.x = projCoords.x * 0.5f + 0.5f;
    texCoords.y = -projCoords.y * 0.5f + 0.5f;
    
    if (texCoords.x < 0.0f || texCoords.x > 1.0f || 
        texCoords.y < 0.0f || texCoords.y > 1.0f)
    {
        return 1.0f;
    }


    float currentDepth = projCoords.z;
    
    if (currentDepth < 0.0f)
    {
        return 1.0f;
    }
    
    float bias = 0.001f;
    
    // 텍셀 크기 계산
    float2 texelSize = 1.0f / 2048.0f;  // SHADOW_MAP_SIZE

	// 3x3 PCF
    float shadow = 0.0f;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float sampleDepth = shadowMap.Sample(samLinear, texCoords + offset).r;
            
            // 오른손 좌표계 비교
            shadow += (currentDepth - bias > sampleDepth) ? 0.0f : 1.0f;
        }
    }
    
    shadow /= 9.0f;
	return lerp(0.3f, 1.0f, shadow);
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 ambient = ambientStr * vAmbientColor;
    
    float3 normalTex = normalMap.Sample(samLinear, input.Tex).xyz * 2.0f - 1.0f; //정규화


	float3 N = normalize(input.Norm);
  

	float3 T = normalize(input.Tan);
	T = normalize(T - dot(T, N) * N); // N에 직교하도록 조정
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
    
    normalTex = normalize(mul(normalTex, TBN)); //TBN 행렬을 곱해서 월드공간으로 변환
    
    N = lerp(N, normalTex, (textureFlags & 4) != 0);
    float3 I = normalize(input.WorldPos - CameraPos.xyz);
    float3 R = reflect(I, N);  //큐브맵 반사를 위한 리플렉트 벡터
    R.x = -R.x;
    float3 L = -vLightDir.xyz;
    
    // 조명 위치와 픽셀 위치
    float3 toLight = vLightPos - input.WorldPos;
    float distance = length(toLight);

    // 감쇠 계수 (1 / d² 형태)
    //float attenuation = 1.0f / (distance * distance);
    
    float lightDist = 1.0f;
    
	float shadow = CalculateShadow(input.LightPos); // 그림자 인자 계산

    float diff = saturate(dot(N, L));
	//float bandLevel = 1.0f;
	//diff = ceil(diff * bandLevel)/bandLevel;
	//diff = lutMap.Sample(samLinear, float2((diff * 0.5f) + 0.495f,0.5f)).r;
    float4 diffuse = diffuseStr * diff * vLightColor * lightDist * shadow;
    
    float3 viewDir = normalize(CameraPos.xyz - input.WorldPos);
    float3 halfDir = normalize(viewDir + L); //스펙큘러연산을 위한 하프 벡터
    
    float specTex = lerp(1.0f, specularMap.Sample(samLinear, input.Tex).r,(textureFlags & 2) != 0);
    float spec = pow(saturate(dot(halfDir, N)), shininess) * sqrt(diff); // * sqrt(diff) <- 이걸 쓰면 shininess < 32 에서 아티팩트가 사라짐..!!! 
    
	//spec = smoothstep(0.01, 0.02f, spec); // <- 카툰렌더링용 스펙큘러
	float4 specular = specularStr * specTex * spec * vLightColor * lightDist * shadow;
    
	float4 emmisive = lerp(float4(0, 0, 0, 0), emmisiveMap.Sample(samLinear, input.Tex),(textureFlags & 8) != 0);

    // 알파 클리핑용 디퓨즈 샘플링
    float4 baseTex = lerp(baseColor, txDiffuse.Sample(samLinear, input.Tex),(textureFlags & 1) != 0);

    // 알파 임계값 설정 (0.1~0.5 정도 보통 사용)
    const float alphaCutoff = 0.5f;

    // 알파가 낮으면 픽셀 폐기
    clip(baseTex.a - alphaCutoff);
    
    float3 baseRGB = (specular + diffuse + ambient).rgb * baseTex.rgb + emmisive.rgb;
    float3 envRGB = (specular + diffuse + ambient).rgb * skyBoxTX.Sample(samLinear, R).rgb;
    
    float3 finalRGB = lerp(baseRGB, envRGB, reflectionFactor);

    return float4(finalRGB, baseTex.a);
}
)";

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
				L"블린 퐁 픽셀 셰이더가 컴파일되지 않았습니다.", L"오류", MB_OK);
			return;
		}

		if (pErrorBlob)
			pErrorBlob->Release();

		hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
			nullptr, s_pBlinnPhongPixelShader.GetAddressOf());
		pPSBlob->Release();

		if (FAILED(hr))
		{
			MessageBox(nullptr, L"픽셀 셰이더 생성 실패", L"오류", MB_OK);
			return;
		}
	}
}

void MyEngine::Material::ReleaseBlinnPhongShaders()
{
	s_pBlinnPhongVertexShader = nullptr;
	s_pBlinnPhongVertexShader_useRigidBone = nullptr;
	s_pBlinnPhongVertexShader_useSkinningBone = nullptr;
	s_pBlinnPhongPixelShader = nullptr;
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
