#include "TextureManager.h"
#include <stdexcept>

void MyEngine::D3DCTX::TextureManager::StartUp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP;

	// ====== 선형 매핑 샘플러 생성 ====== //
	ComPtr<ID3D11SamplerState> pLinearSampler;
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = filter;
	sampDesc.AddressU = addressMode;
	sampDesc.AddressV = addressMode;
	sampDesc.AddressW = addressMode;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = pDevice->CreateSamplerState(&sampDesc, pLinearSampler.GetAddressOf());
	if (FAILED(hr))
		throw std::runtime_error("Failed to create SamplerState.");

	m_pLinearSampler = pLinearSampler;

	// ====== 포인트 매핑 샘플러 생성 ====== //
	filter = D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;

	ComPtr<ID3D11SamplerState> pPointSampler;
	sampDesc.Filter = filter;

	hr = pDevice->CreateSamplerState(&sampDesc, pPointSampler.GetAddressOf());
	if (FAILED(hr))
		throw std::runtime_error("Failed to create SamplerState.");

	m_pPointSampler = pPointSampler;
}

void MyEngine::D3DCTX::TextureManager::ShutDown()
{
	m_pLinearSampler = nullptr;
	m_pPointSampler = nullptr;
}
