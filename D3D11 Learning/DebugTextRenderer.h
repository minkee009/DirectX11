#pragma once
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <directxmath.h>
#include <wrl/client.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyEngine
{
	class DebugTextRenderer
	{
	private:
		HWND m_hWnd;
		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pContext;
	public:
	};
}