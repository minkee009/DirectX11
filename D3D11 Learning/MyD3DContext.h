#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <directxmath.h>

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;

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
		ID3D11Device* p_d3dDevice = NULL;
		ID3D11Device1* p_d3dDevice1 = NULL;
		ID3D11DeviceContext* p_immediateContext = NULL;
		IDXGISwapChain1* p_swapChain1 = NULL;
		IDXGISwapChain* p_swapChain = NULL;
		ID3D11RenderTargetView* p_renderTargetView = NULL;

		D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;
		D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

		//Scene 관련 변수
		ID3D11VertexShader* p_vertexShader = NULL;
		ID3D11PixelShader* p_pixelShader = NULL;
		ID3D11InputLayout* p_vertexLayout = NULL;
		ID3D11Buffer* p_vertexBuffer = NULL;
		ID3D11Buffer* p_indexBuffer = NULL;

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
