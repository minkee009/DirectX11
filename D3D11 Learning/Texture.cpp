#include "Texture.h"

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
		hr = LoadFromTGAFile(path.c_str(), use_sRGB ? TGA_FLAGS_DEFAULT_SRGB : TGA_FLAGS_NONE, &metadata, image);
	}
	else
	{
		hr = LoadFromWICFile(path.c_str(), use_sRGB ? WIC_FLAGS_DEFAULT_SRGB : WIC_FLAGS_NONE, &metadata, image);
	}

	if (FAILED(hr))
		return false;

	ID3D11Device* device = nullptr;
	context->GetDevice(&device);

	ComPtr<ID3D11ShaderResourceView> pSRV;
	hr = CreateShaderResourceView(device, image.GetImages(), image.GetImageCount(), metadata, pSRV.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	pSRV->GetDesc(&srvDesc);

	auto descInfo = srvDesc.Format;

	device->Release();
	if (FAILED(hr))
		return false;

	m_pSRV = pSRV;
	m_name = name;

	m_samplerBinding = TextureSamplerBinding::GlobalSetup;

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
		hr = DirectX::LoadFromTGAMemory(pData, dataSize, use_sRGB ? TGA_FLAGS_DEFAULT_SRGB : TGA_FLAGS_NONE, &metadata, image);
	}
	else // 대부분의 WIC 포맷 (png, jpg, bmp, tiff 등)
	{
		// WIC 포맷은 LoadFromWICMemory 사용
		hr = DirectX::LoadFromWICMemory(pData, dataSize, use_sRGB ? WIC_FLAGS_DEFAULT_SRGB : WIC_FLAGS_NONE, &metadata, image);
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
		metadata,
		pSRV.GetAddressOf());

	device->Release();
	if (FAILED(hr))
		return false;

	// 엔진 구조에 저장
	m_name = name;
	m_pSRV = pSRV;

	m_samplerBinding = TextureSamplerBinding::GlobalSetup;

	return true;
}
