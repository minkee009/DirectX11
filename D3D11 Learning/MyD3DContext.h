#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr

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
		ComPtr<ID3D11Device> p_d3dDevice = nullptr;
		ComPtr<ID3D11Device1> p_d3dDevice1 = nullptr;
		ComPtr<ID3D11DeviceContext> p_immediateContext = nullptr;
		ComPtr<IDXGISwapChain1> p_swapChain1 = nullptr;
		ComPtr<IDXGISwapChain> p_swapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> p_renderTargetView = nullptr;

		D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;
		D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

		//Scene 관련 변수
		ComPtr<ID3D11VertexShader> p_vertexShader = nullptr;
		ComPtr<ID3D11PixelShader> p_pixelShader = nullptr;
		ComPtr<ID3D11InputLayout> p_vertexLayout = nullptr;
		ComPtr<ID3D11Buffer> p_vertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> p_indexBuffer = nullptr;

		UINT m_vertexCount = 0;
		UINT m_vertexBufferStride = 0;
		UINT m_vertexBufferOffset = 0;

		UINT m_indexCount = 0;

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
