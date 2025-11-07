#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr

#include "Camera.h"
#include "AssimpConverter.h"
#include "MeshRenderer.h"
#include "BVH.h"

#include <directxtk/CommonStates.h>
#include <directxtk/Effects.h>
#include "DebugDraw.h"

//#ifdef _DEBUG
#include "MyImGui.h"
//#endif

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyEngine {

	struct SkyBoxVertex {
		XMFLOAT3 pos;
	};

	struct MyVertex {
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT2 uv;
		UINT boneIndices[4];
		float boneWeights[4];
	};

	struct MyConstantBuffer {
		XMMATRIX mWorld;
		XMMATRIX mView;
		XMMATRIX mProjection;
		XMFLOAT3 CameraPos;
		FLOAT pad1;
		XMFLOAT3 vLightPos;
		FLOAT pad2;
		XMFLOAT4 vLightDir;
		XMFLOAT4 vLightColor;
		XMFLOAT4 vOutputColor;
		XMFLOAT4 vAmbientColor;
		FLOAT ambientStr;
		FLOAT diffuseStr;
		FLOAT specularStr;
		UINT shininess;
		FLOAT reflectionFactor;
		XMFLOAT3 pad3;
		XMMATRIX mlightViewProj;
		FLOAT lowLut;
		FLOAT diffGradientDistHalf;
		FLOAT diffGradientDepth;
		FLOAT rimLightStr;
	};

	struct OutlineCB
	{
		FLOAT Thickness;
		XMFLOAT3 pad;
	};

	struct GradientCB
	{
		XMFLOAT4 ColorTop;
		XMFLOAT4 ColorBottom;
		XMFLOAT3 GradientPos;
		FLOAT intensity;
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
		ComPtr<ID3D11DeviceContext> m_pContext = nullptr;
		ComPtr<IDXGISwapChain1> m_pSwapChain1 = nullptr;
		ComPtr<IDXGISwapChain> m_pSwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> m_pRenderTargetView = nullptr;
		ComPtr<ID3D11Texture2D> m_pDepthStencil = nullptr;
		ComPtr<ID3D11DepthStencilView> m_pDepthStencilView = nullptr;

		D3D11_VIEWPORT m_vp;

		D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;
		D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

		ComPtr<ID3D11RasterizerState> m_pDefRasterizerState = nullptr;			//시계방향 컬링 (기본)
		ComPtr<ID3D11RasterizerState> m_pClockWiseRasterizerState = nullptr;		//반시계방향 컬링 (스카이 박스용)
		ComPtr<ID3D11RasterizerState> m_pShadowMapRasterizerState = nullptr;
		ComPtr<ID3D11SamplerState> m_pSamplerLinear = nullptr;
		ComPtr<ID3D11SamplerState> m_pSamplerPoint = nullptr;
		ComPtr<ID3D11BlendState> m_pBlendState = nullptr;
		ComPtr<ID3D11DepthStencilState> m_pOpaqueState = nullptr;
		ComPtr<ID3D11DepthStencilState> m_pTransparentState = nullptr;

		//Scene 관련 변수
		ComPtr<ID3D11VertexShader> m_pVertexShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pPixelShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pPixelShaderSolid = nullptr;
		ComPtr<ID3D11VertexShader> m_pSkyBoxVShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pSkyBoxPShader = nullptr;
		ComPtr<ID3D11InputLayout> m_pCubeInputLayout = nullptr;
		ComPtr<ID3D11InputLayout> m_pSkyBoxInputLayout = nullptr;

		ComPtr<ID3D11Buffer> m_pVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pIndexBuffer = nullptr;

		ComPtr<ID3D11Buffer> m_pSkyBoxVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pSkyBoxIndexBuffer = nullptr;

		ComPtr<ID3D11Buffer> m_pConstantBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pOutlineCB = nullptr;
		ComPtr<ID3D11Buffer> m_pGradientCB = nullptr;

		ComPtr<ID3D11ShaderResourceView> m_pCubeTextureRV = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_pCubeNormalMapRV = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_pCubeSpecularMapRV = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_pSkyBoxTextureRV = nullptr;

		std::unique_ptr<Camera> m_pCamera;
		std::vector<std::unique_ptr<Transform>> m_sceneObjects;
		std::vector<std::unique_ptr<MeshRenderer>> m_meshRenderers;

		bool m_sceneHasChanged = false;
		bool m_usingBVH = true;

		std::unique_ptr<BVH> m_pBVHTree;
		std::vector<BoundingBox> m_bboxRegistry;

		int m_mappedIdx = 0;

		UINT m_currentRenderPassNum = 0;

		std::unique_ptr<Transform> m_pDirectionalLightT;
		XMFLOAT4 m_lightColor = { 1,0.988f,0.952f,1 };
		FLOAT m_lightDistance = 3.0f;
		FLOAT m_lightProjectNear = 0.01f;
		FLOAT m_lightProjectFar = 50.0f;

		XMFLOAT4 m_ambientColor = { 0.85f,0.93f,1,1 };
		FLOAT m_ambientStrength = 0.4f;
		FLOAT m_diffuseStrength = 1.0f;
		FLOAT m_diffuseGradientStrength = 0.3125f;
		FLOAT m_specularStrength = 0.228f;
		FLOAT m_rimLightStrength = 1.0f;
		UINT m_shininess = 512;

		FLOAT m_outlineThickness = 0.02f;

		XMFLOAT4 m_gradientColorTop = { 0.674f, 0.602f, 0.743f, 1.0f };
		XMFLOAT4 m_gradientColorBottom = { 0.674f, 0.602f, 0.743f, 1.0f };
		FLOAT m_gradientIntensity = 1.0f;

		FLOAT m_reflectionFactor = 0.005f;
		bool m_isPointLight = false;

		UINT m_vertexCount = 0;
		UINT m_vertexBufferStride = 0;
		UINT m_vertexBufferOffset = 0;

		UINT m_skyBoxVertexCount = 0;
		UINT m_skyBoxVertexBufferStride = 0;
		UINT m_skyBoxVertexBufferOffset = 0;

		UINT m_indexCount = 0;
		UINT m_skyBoxIndexCount = 0;

		const UINT SHADOW_MAP_SIZE = 4096;

		ComPtr<ID3D11Texture2D> m_pShadowTex;
		ComPtr<ID3D11ShaderResourceView> m_pShadowSRV;
		ComPtr<ID3D11DepthStencilView> m_pShadowDSV;
		D3D11_VIEWPORT m_shadowViewport;
		ComPtr<ID3D11VertexShader> m_pShadowMapVS;
		ComPtr<ID3D11SamplerState> m_pShadowSampler;

		const float SHADOW_MAP_DEPTH = 25.0f;

		// ================ Debug Draw
		using VertexType = DirectX::VertexPositionColor;

		std::unique_ptr<DirectX::CommonStates> m_states;
		std::unique_ptr<DirectX::BasicEffect> m_effect;
		std::unique_ptr<DirectX::PrimitiveBatch<VertexType>> m_batch;
		ComPtr<ID3D11InputLayout> m_pDebugDrawIL;

		bool m_enableDebugDraw = true;
		bool m_enableDebugDrawZbuffer = false;
		// ================

//#ifdef _DEBUG
		//GUI용 코드 (디버깅 용)
		friend class MyImGui;
		MyImGui m_imgui;
//#endif //_DEBUG

		bool InitCube();
		bool InitSkyBox();
		bool InitShadowMapTex();

		void DrawSkyBox();
		void DrawCube();
		void DrawShadowMap();

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
