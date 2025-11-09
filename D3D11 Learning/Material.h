#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <directxtk/SimpleMath.h>
#include <string>

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
using namespace SimpleMath;
using namespace Microsoft::WRL;

namespace MyEngine 
{
	enum class TextureType 
	{
		None = 0,
		Diffuse = 1 << 0,
		Specular = 1 << 1,
		Normal = 1 << 2,
		Emissive = 1 << 3,
		Height = 1 << 4,
		AmbientOcclusion = 1 << 5,
		Roughness = 1 << 6,
		Metalness = 1 << 7,
		CubeMap = 1 << 8
	}; 

	struct TextureBinding
	{
		TextureType type;
		std::string name;
		UINT slot;
		ComPtr<ID3D11ShaderResourceView> pSRV;
	};

	enum class ShaderType
	{
		Vertex,
		Pixel,
		//Geometry,
		//Hull,
		//Domain,
		//Compute
	};

	enum class RenderType
	{
		Opaque,
		Transparent,
	};

	struct MaterialCB
	{
		UINT textureFlags; // bit0=Diffuse, bit1=Specular, bit2=Normal, ...
		float padding[3];  // 16byte align
		Color baseColor;
	};

	class Material
	{
	private:	
		std::string m_name = "";
		UINT m_textureFlags = 0; // 각 TextureType에 해당하는 bitmask
		ComPtr<ID3D11Buffer> m_materialCB; // 상수버퍼
		ComPtr<ID3D11VertexShader> m_pVertexShader;
		ComPtr<ID3D11PixelShader>  m_pPixelShader;
		ComPtr<ID3DBlob> m_pVSBlob;
		ComPtr<ID3D11SamplerState> m_pSampler;

		std::vector<TextureBinding> m_textures;

		Color m_baseColor = { 1,1,1,1 };

		bool m_useZWrite = true;
		bool m_useAlphaBlend = false;
		bool m_useBackFaceCulling = true;

		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
		
		static ComPtr<ID3D11VertexShader> s_pDefaultVertexShader;
		static ComPtr<ID3D11PixelShader>  s_pDefaultPixelShader;
		static ComPtr<ID3D11VertexShader> s_pOutlineVertexShader;
		static ComPtr<ID3D11VertexShader> s_pOutlineVertexShader_useRigidBone;
		static ComPtr<ID3D11VertexShader> s_pOutlineVertexShader_useSkinningBone;
		static ComPtr<ID3D11PixelShader>  s_pOutlinePixelShader;
		static ComPtr<ID3DBlob> s_pDefaultVSBlob;

		static ComPtr<ID3D11VertexShader> s_pBlinnPhongVertexShader;
		static ComPtr<ID3D11VertexShader> s_pBlinnPhongVertexShader_useRigidBone;
		static ComPtr<ID3D11VertexShader> s_pBlinnPhongVertexShader_useSkinningBone;
		static ComPtr<ID3D11PixelShader>  s_pBlinnPhongPixelShader;
		static ComPtr<ID3D11PixelShader>  s_pBlinnPhongToonPixelShader;
		static ComPtr<ID3D11PixelShader>  s_pBlinnPhongShadowMapPixelShader;
		static ComPtr<ID3DBlob> s_pBlinnPhongVSBlob;
	public:
		Material() = default;
		Material(const std::string& name);
		~Material();

		void Bind(ID3D11DeviceContext* context);

		bool InitAndCompileShader(ID3D11Device* device, ShaderType type, const std::wstring& path);
		bool InitShader(ShaderType type, ID3D11DeviceChild* shader, ID3DBlob* vsBlob);
		bool InitSampler(ID3D11Device* device,D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP);
		bool InitSampler(ID3D11SamplerState* pSampler);
		bool InitAndConvertTexture(ID3D11DeviceContext* context, TextureType type, const std::string& name, UINT slot, const std::wstring& path);
		bool InitAndConvertTextureFromMemory(ID3D11DeviceContext* context, TextureType type, const std::string& name, UINT slot, const uint8_t* pData,size_t dataSize, const std::wstring& formatExt);
		bool InitTexture(const std::string& name, TextureType type, UINT slot, ID3D11ShaderResourceView* textureView);

		void CreateConstantBuffer(ID3D11DeviceContext* context);

		inline ID3DBlob* GetVSBlob() const { return m_pVSBlob.Get(); }
		inline ID3D11VertexShader* GetVertexShader() const { return m_pVertexShader.Get(); }
		inline ID3D11PixelShader* GetPixelShader() const { return m_pPixelShader.Get(); }
		inline const std::string& GetName() const { return m_name; }
		inline const Color& GetBaseColor() const { return m_baseColor; }

		inline void SetBaseColor(const Color& baseColor) { m_baseColor = baseColor; }

		static bool CompileLiteralCodeToVertexShader(ID3D11Device* pDevice, ID3D11VertexShader** ppVS, const char* literal, ID3DBlob** ppVSBlob = nullptr);
		static bool CompileLiteralCodeToPixelShader(ID3D11Device* pDevice, ID3D11PixelShader** ppPS, const char* literal);

		//기본 셰이더 (분홍색)
		static void InitDefaultShaders(ID3D11Device* device);
		static void ReleaseDefaultShaders();
		static void BindDefaultShaders(ID3D11DeviceContext* context);
		static void BindOutlineShaders(ID3D11DeviceContext* context);
		inline static ID3D11VertexShader* GetDefaultVertexShader() { return s_pDefaultVertexShader.Get(); }
		inline static ID3D11VertexShader* GetOutlineVertexShader_RigidBone() { return s_pOutlineVertexShader_useRigidBone.Get(); }
		inline static ID3D11VertexShader* GetOutlineVertexShader_SkinningBone() { return s_pOutlineVertexShader_useSkinningBone.Get(); }
		inline static ID3D11PixelShader* GetDefaultPixelShader() { return s_pDefaultPixelShader.Get(); }
		inline static ID3D11VertexShader* GetOutlineVertexShader() { return s_pOutlineVertexShader.Get(); }
		inline static ID3D11PixelShader* GetOutlinePixelShader() { return s_pOutlinePixelShader.Get(); }
		inline static ID3DBlob* GetDefaultVSBlob() { return s_pDefaultVSBlob.Get(); }

		//Blinn Phong 셰이더
		static void InitBlinnPhongShaders(ID3D11Device* device);
		static void ReleaseBlinnPhongShaders();
		inline static ID3D11VertexShader* GetBlinnPhongVertexShader() { return s_pBlinnPhongVertexShader.Get(); }
		inline static ID3D11VertexShader* GetBlinnPhongVertexShader_RigidBone() { return s_pBlinnPhongVertexShader_useRigidBone.Get(); }
		inline static ID3D11VertexShader* GetBlinnPhongVertexShader_SkinningBone() { return s_pBlinnPhongVertexShader_useSkinningBone.Get(); }
		inline static ID3D11PixelShader* GetBlinnPhongPixelShader() { return s_pBlinnPhongPixelShader.Get(); }
		inline static ID3D11PixelShader* GetBlinnPhongToonPixelShader() { return s_pBlinnPhongToonPixelShader.Get(); }
		inline static ID3DBlob* GetBlinnPhongVSBlob() { return s_pBlinnPhongVSBlob.Get(); }
	};
}