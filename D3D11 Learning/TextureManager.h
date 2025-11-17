#pragma once
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include "Singleton.h"

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

namespace MyEngine::D3DCTX
{
	//텍스쳐에서 필요한 공용 자원 (프로그램이 끝날 때까지 유지하는 항목) 관리
	class TextureManager : public Singleton<TextureManager>
	{
	private:
		ComPtr<ID3D11SamplerState> m_pLinearSampler;
		ComPtr<ID3D11SamplerState> m_pPointSampler;
	public:
		void StartUp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
		void ShutDown();

		inline ID3D11SamplerState* GetLinearSampler() { return m_pLinearSampler.Get(); }
		inline ID3D11SamplerState* GetPointSampler() { return m_pPointSampler.Get(); }
	};
}