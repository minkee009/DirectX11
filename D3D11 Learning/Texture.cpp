#include "Texture.h"

bool MyEngine::Texture::SetSamplerState(ID3D11SamplerState* pSampler)
{
    if (pSampler)
    {
        m_pSampler = pSampler;
        return true;
    }

    return false;
}

bool MyEngine::Texture::LoadTextureFromFile(ID3D11DeviceContext* context, const std::string& name, const std::wstring& path)
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

	m_pSRV = pSRV;
	m_name = name;

	return true;
}

bool MyEngine::Texture::LoadTextureFromMemory(ID3D11DeviceContext* context, const std::string& name, const uint8_t* pData, size_t dataSize, const std::wstring& formatExt)
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
	m_name = name;
	m_pSRV = pSRV;

	return true;
}
