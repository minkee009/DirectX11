#pragma once
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include "Singleton.h"

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

namespace MyEngine
{
	struct SkyBoxVertex {
		Vector3 position;
	};

	struct DefaultVertex
	{
		Vector3 position;
		Vector3 normal;
		Vector3 tangent;
		Vector2 uv;
		UINT boneIndices[4];
		float boneWeights[4];
	};

	namespace D3DCTX
	{
		class InputLayoutManager : public Singleton<InputLayoutManager>
		{
		public:
			inline ID3D11InputLayout* GetDefaultInputLayout() { return m_pDefaultInputLayout.Get(); }
			void StartUp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
			void ShutDown();
		private:
			ComPtr<ID3D11InputLayout> m_pDefaultInputLayout;
		};
	}
}


