#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr

#ifdef _DEBUG
#include "MyImGui.h"
#endif

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyEngine {

	struct MyVertex {
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};

	class MyD3DContext {
	private:
		//윈도우 관리 변수
		HWND m_hWnd = nullptr;
		int m_width = 800;
		int m_height = 600;

		//Direct3D 관련 변수
		ComPtr<ID3D11Device> m_pD3DDevice = nullptr;
		ComPtr<ID3D11Device1> m_pD3DDevice1 = nullptr;
		ComPtr<ID3D11DeviceContext> m_pImmediateContext = nullptr;
		ComPtr<IDXGISwapChain1> m_pSwapChain1 = nullptr;
		ComPtr<IDXGISwapChain> m_pSwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> m_pRenderTargetView = nullptr;

		D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;
		D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

		//Scene 관련 변수
		ComPtr<ID3D11VertexShader> m_pVertexShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pPixelShader = nullptr;
		ComPtr<ID3D11InputLayout> m_pVertexLayout = nullptr;
		ComPtr<ID3D11Buffer> m_pVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pIndexBuffer = nullptr;

		UINT m_vertexCount = 0;
		UINT m_vertexBufferStride = 0;
		UINT m_vertexBufferOffset = 0;

		UINT m_indexCount = 0;

#ifdef _DEBUG
		//GUI용 코드 (디버깅 용)
		MyImGui m_imgui;
#endif //_DEBUG

		void Clear();
		void Present();

		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

	public:
		bool Initialize(HWND hWnd, int width, int height);
		
		bool InitializeScene();
		void UninitializeScene();

		void Render();

		~MyD3DContext();
	};
}
