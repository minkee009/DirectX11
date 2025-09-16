#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr

#include "Camera.h"

#ifdef _DEBUG
#include "MyImGui.h"
#endif

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyEngine {

	struct SkyBoxVertex {
		XMFLOAT3 pos;
	};

	struct MyVertex {
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};

	struct MyConstantBuffer {
		XMMATRIX mWorld;
		XMMATRIX mView;
		XMMATRIX mProjection;
	};

	class MyD3DContext {
	private:
		//윈도우 관리 변수
		HWND m_hWnd = nullptr;
		int m_width = 800;
		int m_height = 600;

		//Direct3D 관련 변수
		ComPtr<ID3D11Device> m_pd3dDevice = nullptr;
		ComPtr<ID3D11Device1> m_pd3dDevice1 = nullptr;
		ComPtr<ID3D11DeviceContext> m_pImmediateContext = nullptr;
		ComPtr<IDXGISwapChain1> m_pSwapChain1 = nullptr;
		ComPtr<IDXGISwapChain> m_pSwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> m_pRenderTargetView = nullptr;
		ComPtr<ID3D11Texture2D> m_pDepthStencil = nullptr;
		ComPtr<ID3D11DepthStencilView> m_pDepthStencilView = nullptr;

		D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;
		D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

		ComPtr<ID3D11RasterizerState> m_pDefRasterizerState = nullptr;			//시계방향 컬링 (기본)
		ComPtr<ID3D11RasterizerState> m_pClockWiseRasterizerState = nullptr;		//반시계방향 컬링 (스카이 박스용)
		ComPtr<ID3D11SamplerState> m_pSamplerLinear = nullptr;

		//Scene 관련 변수
		ComPtr<ID3D11VertexShader> m_pVertexShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pPixelShader = nullptr;
		ComPtr<ID3D11VertexShader> m_pSkyBoxVShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pSkyBoxPShader = nullptr;
		ComPtr<ID3D11InputLayout> m_pCubeInputLayout = nullptr;
		ComPtr<ID3D11InputLayout> m_pSkyBoxInputLayout = nullptr;

		ComPtr<ID3D11Buffer> m_pVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pIndexBuffer = nullptr;

		ComPtr<ID3D11Buffer> m_pSkyBoxVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pSkyBoxIndexBuffer = nullptr;

		ComPtr<ID3D11Buffer> m_pConstantBuffer = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_pCubeTextureRV = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_pSkyBoxTextureRV = nullptr;

		std::unique_ptr<Camera> m_pCamera;
		std::vector<std::unique_ptr<Transform> > m_sceneObjects;

		XMFLOAT4 m_lightDirs[2] = { {1,0,0,1},{0,1,0,1} };
		XMFLOAT4 m_lightColors[2] = { {1,1,1,1},{1,0,0,1} };

		UINT m_vertexCount = 0;
		UINT m_vertexBufferStride = 0;
		UINT m_vertexBufferOffset = 0;

		UINT m_skyBoxVertexCount = 0;
		UINT m_skyBoxVertexBufferStride = 0;
		UINT m_skyBoxVertexBufferOffset = 0;

		UINT m_indexCount = 0;
		UINT m_skyBoxIndexCount = 0;

#ifdef _DEBUG
		//GUI용 코드 (디버깅 용)
		friend class MyImGui;
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

		void Resize(UINT width, UINT height);

		~MyD3DContext();
	};
}
