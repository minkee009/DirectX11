#pragma once
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include "Singleton.h"

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

namespace MyEngine::D3DCTX
{
	class TextureManager : public Singleton<TextureManager>
	{

	};
}