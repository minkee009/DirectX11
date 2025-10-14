#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <string>

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyEngine {
	//enum class TextureType 
	//{
	//	None = 0,
	//	Diffuse = 1 << 0,
	//	Specular = 1 << 1,
	//	Normal = 1 << 2,
	//	Emissive = 1 << 3,
	//	Height = 1 << 4,
	//	AmbientOcclusion = 1 << 5,
	//	Roughness = 1 << 6,
	//	Metalness = 1 << 7,
	//	CubeMap = 1 << 8
	//};

	struct TextureBinding 
	{
		std::wstring name;
		//TextureType type;

		UINT slot;
		ComPtr<ID3D11ShaderResourceView> pSRV;
		ComPtr<ID3D11SamplerState> pSampler;
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
		static ComPtr<ID3D11VertexShader> s_defaultVertexShader;
		static ComPtr<ID3D11PixelShader>  s_defaultPixelShader;

		static ComPtr<ID3DBlob> s_defaultVSBlob;

		ComPtr<ID3D11VertexShader> m_vertexShader;
		ComPtr<ID3D11PixelShader>  m_pixelShader;

		ComPtr<ID3DBlob> m_vsBlob;

		std::vector<TextureBinding> textures;

		HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	public:
		Material(const std::wstring& name) : name(name) { }

		void Bind(ID3D11DeviceContext* context);

		bool InitAndCompileShader(ID3D11Device* device, ShaderType type, const std::wstring& path);
		bool InitShader(ShaderType type, ID3D11DeviceChild* shader);
		bool InitTexture(ID3D11DeviceContext* ctx, const std::wstring& name, UINT slot, const std::wstring& path,
			D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP);

		const std::wstring name;
		inline ID3DBlob* GetVSBlob() const { return m_vsBlob.Get(); }
		inline ID3D11VertexShader* GetVertexShader() const { return m_vertexShader.Get(); }
		inline ID3D11PixelShader* GetPixelShader() const { return m_pixelShader.Get(); }

		//정적 기본 셰이더 설정
		static void InitDefaultShaders(ID3D11Device* device);
		static void ReleaseDefaultShaders();
		inline static ID3D11VertexShader* GetDefaultVertexShader() { return s_defaultVertexShader.Get(); }
		inline static ID3D11PixelShader* GetDefaultPixelShader() { return s_defaultPixelShader.Get(); }
		inline static ID3DBlob* GetDefaultVSBlob() { return s_defaultVSBlob.Get(); }
	};
}