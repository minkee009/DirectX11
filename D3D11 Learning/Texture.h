#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <string>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace MyEngine
{
	/// <summary>
	/// 공유 텍스쳐를 위한 메타 데이터 클래스
	/// 텍스쳐에 대한 참조 접근을 제공
	/// </summary>
	class Texture
	{
	private:
		std::string m_name;
		ComPtr<ID3D11ShaderResourceView> m_pSRV;
		ID3D11SamplerState* m_pSampler;
	public:
		inline const std::string& GetName() const { return m_name; }

		bool SetSamplerState(ID3D11SamplerState* pSampler);
		inline ID3D11SamplerState* GetSamplerState() { return m_pSampler; }
		inline ID3D11SamplerState** GetSamplerStateAddress() { return &m_pSampler; }

		inline ID3D11ShaderResourceView* GetSRV() { return m_pSRV.Get(); }
		inline ID3D11ShaderResourceView** GetSRVAddress() { return m_pSRV.GetAddressOf(); }

		//파일로 부터 텍스쳐 리소스(SRV)를 만드는 함수
		bool LoadTextureFromFile(ID3D11DeviceContext* context, const std::wstring& path);
		bool LoadTextureFromMemory(ID3D11DeviceContext* context, const std::string& name, const uint8_t* pData, size_t dataSize, const std::wstring& formatExt);
	};
}