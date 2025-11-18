#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <directxtk/SimpleMath.h>
#include <string>
#include <memory>

#pragma comment(lib, "d3d11.lib")

#include "Resource.h"
#include "Texture.h"

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
		CubeMap = 1 << 8,
		LookUpTable = 1 << 9
	}; 

	struct TextureBinding
	{
		TextureType type; 
		UINT slot;
		std::shared_ptr<Texture> pTexture;
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

	class Material : public Resource
	{
	private:	
		std::string m_name = "";
		UINT m_textureFlags = 0; // 각 TextureType에 해당하는 bitmask
		ComPtr<ID3D11Buffer> m_materialCB; // 상수버퍼

		ID3D11VertexShader* m_pVertexShader;
		ID3D11PixelShader* m_pPixelShader;

		std::vector<TextureBinding> m_textures;

		bool m_hasBaseColor = false;
		Color m_baseColor = { 1,1,1,1 };

		bool m_useZWrite = true;
		bool m_useAlphaBlend = false;
		bool m_useBackFaceCulling = true;
	public:
		Material() = default;
		Material(const std::string& name);
		~Material();

		void Bind(ID3D11DeviceContext* context);
		bool InitVertexShader(ID3D11VertexShader* shader);
		bool InitPixelShader(ID3D11PixelShader* shader);
		bool InitTexture(TextureType type, UINT slot, std::shared_ptr<Texture> texture);

		void CreateConstantBuffer(ID3D11DeviceContext* context);

		inline const std::string& GetName() const { return m_name; }
		inline const Color& GetBaseColor() const { return m_baseColor; }

		inline void SetBaseColor(const Color& baseColor) { m_hasBaseColor = true; m_baseColor = baseColor; }
	};
}