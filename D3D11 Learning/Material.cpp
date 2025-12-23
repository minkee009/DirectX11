#include "Material.h"

#include <DirectXTex.h>
#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool MyEngine::Material::InitVertexShader(ID3D11VertexShader* shader)
{
	m_pVertexShader = static_cast<ID3D11VertexShader*>(shader);
	return true;
}

bool MyEngine::Material::InitPixelShader(ID3D11PixelShader* shader)
{
	m_pPixelShader = static_cast<ID3D11PixelShader*>(shader);
	return true;
}

bool MyEngine::Material::InitTexture(TextureType type, UINT slot, std::shared_ptr<Texture> texture)
{
	if(!texture)
		return false;

	m_textureFlags |= static_cast<UINT>(type);
	m_textures.push_back(TextureBinding{ type, slot, texture });

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
	cb.propertyFlags = m_propertyFlags;
	cb.baseColor = m_baseColor;
	cb.roughness = m_roughness;
	cb.metallic = m_metallic;
	context->UpdateSubresource(m_materialCB.Get(), 0, nullptr, &cb, 0, 0);
}

MyEngine::Material::Material(const std::string& name)
	: m_name(name)
	, m_pVertexShader(nullptr)
	, m_pPixelShader(nullptr)
{
}

MyEngine::Material::~Material()
{
	m_materialCB = nullptr;
	m_pVertexShader = nullptr;
	m_pPixelShader = nullptr;
	m_textures.clear();
}

void MyEngine::Material::Bind(ID3D11DeviceContext* context, ExcludeShaderFlag flags)
{
	//상수버퍼 설정
	context->PSSetConstantBuffers(1, 1, m_materialCB.GetAddressOf());

	if (m_pVertexShader && (flags & ExcludeShaderFlag::VertexShader) == 0)
		context->VSSetShader(m_pVertexShader, nullptr, 0);
	if (m_pPixelShader && (flags & ExcludeShaderFlag::PixelShader) == 0)
		context->PSSetShader(m_pPixelShader, nullptr, 0);

	for (auto& tex : m_textures)
	{
		if (tex.pTexture->GetSRV())
			context->PSSetShaderResources(tex.slot, 1, tex.pTexture->GetSRVAddress());
		if (tex.pTexture->GetSamplerState())
			context->PSSetSamplers(tex.slot, 1, tex.pTexture->GetSamplerStateAddress());
	}

	if (m_textures.empty() && (m_propertyFlags & static_cast<UINT>(TextureType::Diffuse)) == 0)
	{
		//ShaderManager::Get()->BindDefaultShader();
	}
}
