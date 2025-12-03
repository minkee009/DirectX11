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
		Diffuse = 1 << 0,			// 1
		Specular = 1 << 1,			// 2
		Normal = 1 << 2,			// 4
		Emissive = 1 << 3,			// 8
		Height = 1 << 4,			// 16
		AmbientOcclusion = 1 << 5,	// 32
		Roughness = 1 << 6,			// 64
		Metalness = 1 << 7,			// 128
		CubeMap = 1 << 8,			// 256
		LookUpTable = 1 << 9,		// 512
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
		UINT propertyFlags; // non-texture, keyFlags -> same TextureFlags
		float padding[2];
		Color baseColor;
		float roughness;
		float metallic;
		float padding2[2];
	};

	class Material : public Resource
	{
	private:	
		std::string m_name = "";
		UINT m_textureFlags = 0; // 각 TextureType에 해당하는 bitmask
		UINT m_propertyFlags = 0;
		ComPtr<ID3D11Buffer> m_materialCB; // 상수버퍼

		ID3D11VertexShader* m_pVertexShader = nullptr;
		ID3D11PixelShader* m_pPixelShader = nullptr;

		std::vector<TextureBinding> m_textures;

		Color m_baseColor = { 1,1,1,1 };
		float m_roughness = 0.0f;
		float m_metallic = 0.0f;

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

		inline const UINT& GetTextureFlags() const { return m_textureFlags; }
		inline const std::string& GetName() const { return m_name; }
		inline const Color& GetBaseColor() const { return m_baseColor; }
		inline const float& GetRoughnessKey() const { return m_roughness; }
		inline const float& GetMetallicKey() const { return m_metallic; }

		inline void SetName(std::string&& name) { m_name = name; }
		inline void SetBaseColor(const Color& baseColor) { m_propertyFlags |= static_cast<UINT>(TextureType::Diffuse); m_baseColor = baseColor; }
		inline void SetRoughnessKey(const float& roughness) { m_propertyFlags |= static_cast<UINT>(TextureType::Roughness); m_roughness = m_roughness; }
		inline void SetMetallicKey(const float& metallic) { m_propertyFlags |= static_cast<UINT>(TextureType::Metalness); m_metallic = metallic; }
	};
}