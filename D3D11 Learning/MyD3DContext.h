#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr

#include "StaticBVH.h"

#include "Camera.h"
#include "AssimpConverter.h"
#include "MeshRenderer.h"

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

	struct ObjectMatCB              //b5
	{
		XMMATRIX mWorld;
		UINT isSkinnedMesh;
		XMFLOAT3 pad;
	};

	struct CameraCB					//b6
	{
		XMMATRIX mView;
		XMMATRIX mProjection;
		XMFLOAT3 CameraPos;
		FLOAT pad;
	};

	struct DirectionalLightCB		//b7
	{
		XMFLOAT3 Position;
		FLOAT pad;
		XMFLOAT4 Direction;
		XMFLOAT3 Color;
		FLOAT Intensity;
		XMMATRIX mLightViewProjection;
	};

	struct PointLightCB				//b8
	{
		XMFLOAT3 Color;
		FLOAT Intensity;
		XMFLOAT3 Position;
		FLOAT Range;
	};

	struct PBRDebugCB				//b9
	{
		FLOAT UseOverride;
		FLOAT MetallicOverride;
		FLOAT RoughnessOverride;
		FLOAT AmbeintIntensity;
	};


	struct OutlineCB
	{
		FLOAT Thickness;
		XMFLOAT3 pad;
	};

	struct GradientCB
	{
		XMFLOAT3 GradientPos;
		FLOAT intensity;
	};

	struct PostProcessCB
	{
		FLOAT exposure;
		FLOAT supportHDR;
		XMFLOAT2 pad;
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
		ComPtr<IDXGIDevice3> m_dxgiDevice;
		ComPtr<IDXGISwapChain1> m_pSwapChain1 = nullptr;
		ComPtr<IDXGISwapChain> m_pSwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV = nullptr;
		ComPtr<ID3D11Texture2D> m_pDepthStencilTex = nullptr;
		ComPtr<ID3D11DepthStencilView> m_pDepthStencilView = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_pDepthStencilSRV = nullptr;

		D3D11_VIEWPORT m_vp;

		D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;
		D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

		ComPtr<ID3D11RasterizerState> m_pDefRasterizerState = nullptr;			//시계방향 컬링 (기본)
		ComPtr<ID3D11RasterizerState> m_pClockWiseRasterizerState = nullptr;		//반시계방향 컬링 (스카이 박스용)
		ComPtr<ID3D11RasterizerState> m_pShadowMapRasterizerState = nullptr;
		ComPtr<ID3D11SamplerState> m_pSamplerLinear = nullptr;
		ComPtr<ID3D11SamplerState> m_pSamplerPoint = nullptr;
		ComPtr<ID3D11BlendState> m_pBlendState = nullptr;
		ComPtr<ID3D11BlendState> m_pGeometryBlendState = nullptr;
		ComPtr<ID3D11DepthStencilState> m_pOpaqueState = nullptr;
		ComPtr<ID3D11DepthStencilState> m_pTransparentState = nullptr;

		ComPtr<ID3D11Texture2D> m_pSceneColorTex = nullptr;
		ComPtr<ID3D11RenderTargetView> m_pSceneColorRTV = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_pSceneColorSRV = nullptr;


		ComPtr<ID3D11Buffer> m_pPostProcessCB = nullptr;

		// post processing
		FLOAT m_exposure = 1.0f;

#define GBUFFER_TEX_SIZE 5

		// Deffered Rendering
		//
		// ** Texture Data **
		// 
		//  G-Buffer #1 -> Position
		//  G-Buffer #2 -> Normal
		//  G-Buffer #3 -> Albedo
		//  G-Buffer #4 -> Metallic
		//  G-Buffer #5 -> Roughness
		//
		// -------------------------

		// G-Buffer 포맷 배열
		const DXGI_FORMAT m_GBufferFormats[GBUFFER_TEX_SIZE] =
		{
			DXGI_FORMAT_R16G16B16A16_FLOAT, // #1 Position (FP16: 메모리 절약과 정밀도의 균형)
			DXGI_FORMAT_R8G8B8A8_SNORM,     // #2 Normal (R8G8B8A8_SNORM: 법선 벡터를 위한 메모리 효율적인 형식)
			DXGI_FORMAT_R8G8B8A8_UNORM,     // #3 Albedo (R8G8B8A8_UNORM: 표준 색상)
			DXGI_FORMAT_R8_UNORM,           // #4 Metallic (R8_UNORM: 단일 채널)
			DXGI_FORMAT_R8_UNORM            // #5 Roughness (R8_UNORM: 단일 채널)
		};

		ComPtr<ID3D11Texture2D> m_pGBufferTextures[GBUFFER_TEX_SIZE] = { nullptr, };
		ComPtr<ID3D11RenderTargetView> m_pGBufferRTV[GBUFFER_TEX_SIZE] = { nullptr, };
		ComPtr<ID3D11ShaderResourceView> m_pGBufferSRV[GBUFFER_TEX_SIZE] = { nullptr, };

		// Point Light Buffer -> For Deffered Rendering

		struct PointLight
		{
			Vector3 Position;
			Color Color;
			FLOAT LightRange;
			FLOAT Intensity;
		};

		ComPtr<ID3D11Buffer> m_pPointLightCB;
		
		std::vector<PointLight> m_pointLights;

		//Scene 관련 변수
		std::unique_ptr<StaticBVH> m_pBVHTree;
		std::vector<BoundingBox> m_bboxRegistry;

		ComPtr<ID3D11ShaderResourceView> m_pLUTSRV = nullptr;

		ComPtr<ID3D11VertexShader> m_pSkyBoxVShader = nullptr;
		ComPtr<ID3D11PixelShader> m_pSkyBoxPShader = nullptr;
		ComPtr<ID3D11InputLayout> m_pSkyBoxInputLayout = nullptr;

		ComPtr<ID3D11Buffer> m_pSkyBoxVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pSkyBoxIndexBuffer = nullptr;

		ComPtr<ID3D11Buffer> m_pConstantBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pOutlineCB = nullptr;
		ComPtr<ID3D11Buffer> m_pGradientCB = nullptr;

		ComPtr<ID3D11Buffer> m_pObjectMatBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pCameraBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pDirectionalLightBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pPointLightBuffer = nullptr;
		ComPtr<ID3D11Buffer> m_pPBRDebugBuffer = nullptr;

		ComPtr<ID3D11ShaderResourceView> m_pSkyBoxTextureRV = nullptr;

		std::unique_ptr<Camera> m_pCamera;
		std::vector<std::unique_ptr<Transform>> m_sceneObjects;
		std::vector<std::unique_ptr<MeshRenderer>> m_meshRenderers;

		UINT m_currentRenderPassNum = 0;

		std::unique_ptr<Transform> m_pDirectionalLightT;
		XMFLOAT4 m_lightColor = { 1, 0.9411764705882353f,0.8588235294117647f,1 };
		FLOAT m_lightDistance = 3.0f;
		FLOAT m_lightProjectNear = 0.01f;
		FLOAT m_lightProjectFar = 50.0f;

		XMFLOAT4 m_ambientColor = { 0.9255f,0.5059f,0.7490f,1 };
		FLOAT m_ambientStrength = 0.4f;
		FLOAT m_diffuseStrength = 1.0f;
		FLOAT m_diffuseGradientStrength = 0.3125f;
		FLOAT m_specularStrength = 15.0f;
		FLOAT m_rimLightStrength = 1.0f;
		UINT m_shininess = 512;

		FLOAT m_outlineThickness = 0.02f;

		FLOAT m_gradientIntensity = 0.846f;

		FLOAT m_reflectionFactor = 0.005f;
		bool m_isPointLight = false;

		bool m_useMatOverride = false;

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

		// ================ BRDF Environment
		ComPtr<ID3D11ShaderResourceView> m_pIrradianceSRV;          //IBL Diffuse
		ComPtr<ID3D11ShaderResourceView> m_pPrefilteredEnvSRV;		//IBL Specular
		ComPtr<ID3D11ShaderResourceView> m_pEnvSRV;					//IBL Env
		ComPtr<ID3D11ShaderResourceView> m_pBRDFLUTSRV;             //IBL BRDF LUT

		// ================

		bool m_supportHDR = false;

		// ================ Debug Draw
		using DefaultVertex = DirectX::VertexPositionColor;

		std::unique_ptr<DirectX::CommonStates> m_states;
		std::unique_ptr<DirectX::BasicEffect> m_effect;
		std::unique_ptr<DirectX::PrimitiveBatch<DefaultVertex>> m_batch;
		ComPtr<ID3D11InputLayout> m_pDebugDrawIL;

		bool m_enableDebugDraw = false;
		bool m_enableDebugDrawZbuffer = false;
		// ================

//#ifdef _DEBUG
		//GUI용 코드 (디버깅 용)
		friend class MyImGui;
		MyImGui m_imgui;

//#endif //_DEBUG

		bool CreateConstantBuffer(
			ID3D11Device* device,
			UINT size,
			D3D11_USAGE usage,
			UINT cpuAccess,
			ComPtr<ID3D11Buffer>& outBuffer);

		bool InitDefferedRenderpassBuffer();
		void UninitDefferedRenderpassBuffer();

		bool InitSkyBox();
		bool InitShadowMapTex();
		bool InitBRDFEnvironment();

		bool InitGBufferTex();

		void UninitGBufferTex();


		void DrawSkyBox();
		void DrawShadowMap();
		void DrawSkeleton(Transform& t, SkinningMeshRenderer& renderer);

		void CreateSkinningRenderer(const Vector3& pos);

		void Clear();
		void Present();

		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

		bool CheckHDRSupport();

		void ResizeGBufferTex(UINT width, UINT height);

		void ForwardRenderPass();
		void DefferedRenderPass();

	public:
		bool Initialize(HWND hWnd, int width, int height);
		
		bool InitializeScene();
		void UninitializeScene();

		void Update();
		void Render();

		void Resize(UINT width, UINT height);

		~MyD3DContext();
	};
}
