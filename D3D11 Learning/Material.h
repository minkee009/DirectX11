#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <string>

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
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

	class Material
	{
	private:
		static ComPtr<ID3D11VertexShader> s_pDefaultVertexShader;
		static ComPtr<ID3D11PixelShader>  s_pDefaultPixelShader;
		static ComPtr<ID3DBlob> s_pDefaultVSBlob;

		static ComPtr<ID3D11VertexShader> s_pBlinnPhongVertexShader;
		static ComPtr<ID3D11PixelShader>  s_pBlinnPhongPixelShader;
		static ComPtr<ID3DBlob> s_pBlinnPhongVSBlob;

		ComPtr<ID3D11VertexShader> m_pVertexShader;
		ComPtr<ID3D11PixelShader>  m_pPixelShader;
		ComPtr<ID3DBlob> m_pVSBlob;
		ComPtr<ID3D11SamplerState> m_pSampler;

		std::vector<TextureBinding> m_textures;

		const std::string m_name;

		bool m_useZTest = true;
		bool m_useAlphaTest = true;
		bool m_useBackFaceCulling = true;

		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	public:
		Material(const std::string& name) : m_name(name) {}

		void Bind(ID3D11DeviceContext* context);

		bool InitAndCompileShader(ID3D11Device* device, ShaderType type, const std::wstring& path);
		bool InitShader(ShaderType type, ID3D11DeviceChild* shader, ID3DBlob* vsBlob);
		bool InitSampler(ID3D11Device* device,D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP);
		bool InitSampler(ID3D11SamplerState* pSampler);
		bool InitAndConvertTexture(ID3D11DeviceContext* context, TextureType type, const std::string& name, UINT slot, const std::wstring& path);
		bool InitTexture(const std::string& name, TextureType type, UINT slot, ID3D11ShaderResourceView* textureView);

		inline ID3DBlob* GetVSBlob() const { return m_pVSBlob.Get(); }
		inline ID3D11VertexShader* GetVertexShader() const { return m_pVertexShader.Get(); }
		inline ID3D11PixelShader* GetPixelShader() const { return m_pPixelShader.Get(); }

		const std::string& GetName() const { return m_name; }

		//±âº» ¼ÎÀÌ´õ (ºÐÈ«»ö)
		static void InitDefaultShaders(ID3D11Device* device);
		static void ReleaseDefaultShaders();
		static void BindDefaultShaders(ID3D11DeviceContext* context);
		inline static ID3D11VertexShader* GetDefaultVertexShader() { return s_pDefaultVertexShader.Get(); }
		inline static ID3D11PixelShader* GetDefaultPixelShader() { return s_pDefaultPixelShader.Get(); }
		inline static ID3DBlob* GetDefaultVSBlob() { return s_pDefaultVSBlob.Get(); }

		//Blinn Phong ¼ÎÀÌ´õ
		static void InitBlinnPhongShaders(ID3D11Device* device);
		static void ReleaseBlinnPhongShaders();
		inline static ID3D11VertexShader* GetBlinnPhongVertexShader() { return s_pBlinnPhongVertexShader.Get(); }
		inline static ID3D11PixelShader* GetBlinnPhongPixelShader() { return s_pBlinnPhongPixelShader.Get(); }
		inline static ID3DBlob* GetBlinnPhongVSBlob() { return s_pBlinnPhongVSBlob.Get(); }
	};
}