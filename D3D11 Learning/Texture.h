#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <string>

#include "Resource.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace MyEngine
{
	/// <summary>
	/// 공유 텍스쳐를 위한 메타 데이터 클래스
	/// 텍스쳐에 대한 참조 접근을 제공
	/// </summary>
	enum class TextureSamplerBinding
	{
		GlobalSetup,
		Linear,
		Point,
	};
	
	class Texture : public Resource
	{
	private:
		std::string m_name;
		ComPtr<ID3D11ShaderResourceView> m_pSRV;
		TextureSamplerBinding m_samplerBinding;
	public:
		inline const std::string& GetName() const { return m_name; }

		inline ID3D11ShaderResourceView* GetSRV() { return m_pSRV.Get(); }
		inline ID3D11ShaderResourceView** GetSRVAddress() { return m_pSRV.GetAddressOf(); }

		inline const TextureSamplerBinding& GetSamplerBinding() const { return m_samplerBinding; }
		inline void SetSamplerBinding(TextureSamplerBinding&& samplerBinding) { m_samplerBinding = samplerBinding; }


		//파일로 부터 텍스쳐 리소스(SRV)를 만드는 함수
		bool LoadTextureFromFile(ID3D11DeviceContext* context, const std::string& name, const std::wstring& path, bool use_sRGB = false);
		bool LoadTextureFromMemory(ID3D11DeviceContext* context, const std::string& name, const uint8_t* pData, size_t dataSize, const std::wstring& formatExt, bool use_sRGB = false);
	};
}