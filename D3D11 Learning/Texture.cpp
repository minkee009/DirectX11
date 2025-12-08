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

bool MyEngine::Texture::LoadTextureFromFile(ID3D11DeviceContext* context, const std::string& name, const std::wstring& path, bool use_sRGB)
{
	//dds인지 아닌지 확인
	bool isDDS = false;
	
	//tga인지 아닌지 확인
	bool isTGA = false;

	size_t extPos = path.rfind(L'.');
	if (extPos != std::wstring::npos)
	{
		std::wstring ext = path.substr(extPos);
		if (ext == L".dds" || ext == L".DDS")
			isDDS = true;

		else if (ext == L".tga" || ext == L".TGA")
			isTGA = true;
	}

	//-------- 텍스쳐 로드 --------//
	HRESULT hr = S_OK;
	TexMetadata metadata;

	DirectX::ScratchImage image;
	if (isDDS)
	{
		hr = LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, &metadata, image);
	}
	else if (isTGA)
	{
		hr = LoadFromTGAFile(path.c_str(), &metadata, image);
	}
	else
	{
		hr = LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &metadata, image);
	}

	if (FAILED(hr))
		return false;

	ID3D11Device* device = nullptr;
	context->GetDevice(&device);

	if (use_sRGB)
	{
		if (!DirectX::IsSRGB(metadata.format))
		{
			DirectX::ScratchImage srgb_image;

			// 1. 현재 메타데이터에서 목표 sRGB 포맷을 생성합니다.
			DXGI_FORMAT srgbFormat = DirectX::MakeSRGB(metadata.format);

			// 2. Convert 함수를 사용하여 텍스처 포맷을 변경합니다.
			//    (필터링 플래그는 TEX_FILTER_DEFAULT를 사용하거나 필요에 따라 변경)
			hr = DirectX::Convert(
				image.GetImages(),
				image.GetImageCount(),
				metadata,
				srgbFormat,
				DirectX::TEX_FILTER_DEFAULT, // 필터링 옵션 (변환 시 리샘플링을 수행하지 않으므로 기본값 사용)
				0.0f,
				srgb_image
			);

			if (SUCCEEDED(hr))
			{
				// 변환에 성공했다면, 원본 image와 metadata를 새로운 srgb_image로 대체합니다.
				image = std::move(srgb_image);
				metadata = image.GetMetadata(); // metadata.format은 이제 _SRGB 포맷입니다.
			}
			else
			{
				// 변환 실패 처리 (디버깅 필요)
				return false;
			}
		}
	}

	ComPtr<ID3D11ShaderResourceView> pSRV;
	hr = CreateShaderResourceView(device, image.GetImages(), image.GetImageCount(), metadata, pSRV.GetAddressOf());
	device->Release();
	if (FAILED(hr))
		return false;

	m_pSRV = pSRV;
	m_name = name;

	return true;
}

bool MyEngine::Texture::LoadTextureFromMemory(ID3D11DeviceContext* context, const std::string& name, const uint8_t* pData, size_t dataSize, const std::wstring& formatExt, bool use_sRGB)
{
	if (!pData || dataSize == 0)
		return false;

	HRESULT hr = S_OK;
	DirectX::ScratchImage image;
	DirectX::TexMetadata metadata;

	if (formatExt == L".dds" || formatExt == L".DDS")
	{
		// DDS 포맷은 LoadFromDDSMemory 사용
		hr = DirectX::LoadFromDDSMemory(pData, dataSize, DirectX::DDS_FLAGS_NONE, &metadata, image);
	}
	else if (formatExt == L".tga" || formatExt == L".TGA")
	{
		// TGA 포맷은 LoadFromTGAMemory 사용
		hr = DirectX::LoadFromTGAMemory(pData, dataSize, &metadata, image);
	}
	else // 대부분의 WIC 포맷 (png, jpg, bmp, tiff 등)
	{
		// WIC 포맷은 LoadFromWICMemory 사용
		hr = DirectX::LoadFromWICMemory(pData, dataSize, DirectX::WIC_FLAGS_NONE, &metadata, image);
	}

	if (FAILED(hr))
	{
		// 로딩 실패
		return false;
	}

	// D3D11 Device 가져오기
	ComPtr<ID3D11Device> device;
	context->GetDevice(device.GetAddressOf());

	if (use_sRGB)
	{
		if (!DirectX::IsSRGB(metadata.format))
		{
			DirectX::ScratchImage srgb_image;

			// 1. 현재 메타데이터에서 목표 sRGB 포맷을 생성합니다.
			DXGI_FORMAT srgbFormat = DirectX::MakeSRGB(metadata.format);

			// 2. Convert 함수를 사용하여 텍스처 포맷을 변경합니다.
			//    (필터링 플래그는 TEX_FILTER_DEFAULT를 사용하거나 필요에 따라 변경)
			hr = DirectX::Convert(
				image.GetImages(),
				image.GetImageCount(),
				metadata,
				srgbFormat,
				DirectX::TEX_FILTER_DEFAULT, // 필터링 옵션 (변환 시 리샘플링을 수행하지 않으므로 기본값 사용)
				0.0f,
				srgb_image
			);

			if (SUCCEEDED(hr))
			{
				// 변환에 성공했다면, 원본 image와 metadata를 새로운 srgb_image로 대체합니다.
				image = std::move(srgb_image);
				metadata = image.GetMetadata(); // metadata.format은 이제 _SRGB 포맷입니다.
			}
			else
			{
				// 변환 실패 처리 (디버깅 필요)
				return false;
			}
		}
	}

	// ShaderResourceView 생성
	ComPtr<ID3D11ShaderResourceView> pSRV;
	hr = DirectX::CreateShaderResourceView(
		device.Get(),
		image.GetImages(),
		image.GetImageCount(),
		metadata,
		pSRV.GetAddressOf());

	device->Release();
	if (FAILED(hr))
		return false;

	// 엔진 구조에 저장
	m_name = name;
	m_pSRV = pSRV;

	return true;
}
