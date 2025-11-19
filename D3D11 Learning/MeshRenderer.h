#pragma once
#include "IRenderable.h"
#include "Material.h"
#include <unordered_set>
#include <unordered_map>
#include <initializer_list>
#include <DirectXCollision.h>

namespace DebugStatusUI
{
	class MeshRenderer
	{
	public:
		static bool limitDrawOption;
		static int meshNum;
	};
}

namespace MyEngine
{
	class MeshRenderer : public IRenderable
	{
	private:
		UINT m_renderPassNum = 0;
		bool m_enabledBindMeshes = true;
		bool m_enabledBindMaterials = true;
		std::unordered_map<UINT, std::unordered_set<UINT>> m_passExcludedMeshes; // < render pass, { dont draw mesh idx } >
		std::unordered_map<UINT, ID3D11VertexShader*> m_passForceChangeVS;
		std::unordered_map<UINT, ID3D11PixelShader*> m_passForceChangePS;
		std::unordered_set<std::string> m_passCheckKeywords; // 패스에서 문자열 체크할 수 있는 레이블 모음
	protected:
		std::vector<std::shared_ptr<Material>> m_materials;

		inline const std::unordered_map<UINT, std::unordered_set<UINT>>& GetPassExcludedMeshes() const { return m_passExcludedMeshes; };
		inline const std::unordered_map<UINT, ID3D11VertexShader*>& GetPassForceChangeVS() const { return m_passForceChangeVS; };
		inline const std::unordered_map<UINT, ID3D11PixelShader*>& GetPassForceChangePS() const { return m_passForceChangePS; };
		inline const UINT& GetRenderPassNum() const { return m_renderPassNum; }
	public:
		virtual void Draw(ID3D11DeviceContext* context) = 0;
		virtual const BoundingBox& GetBBox() = 0;

		void SetPassExcludedMeshes(UINT renderPassNum, std::initializer_list<UINT> meshes);
		void SetPassForceChangeVS(UINT renderPassNum, ID3D11VertexShader* VS);
		void SetPassForceChangePS(UINT renderPassNum, ID3D11PixelShader* PS);

		inline void AddMaterial(std::shared_ptr<Material> material) { m_materials.push_back(material); }
		inline void SetRenderPassNum(UINT renderPassNum) { m_renderPassNum = renderPassNum; }
		inline bool SetPassCheckKeyword(std::string_view keyword) { return m_passCheckKeywords.insert(std::string(keyword)).second; }

		inline void ClearPassExcludedMeshes() { m_passExcludedMeshes.clear(); }
		inline void ClearPassForceChangeVS() { m_passForceChangeVS.clear(); }
		inline void ClearPassForceChangePS() { m_passForceChangePS.clear(); }

		inline void SetEnabledBindMeshes(bool enabled) { m_enabledBindMeshes = enabled; }
		inline void SetEnabledBindMaterials(bool enabled) { m_enabledBindMaterials = enabled; }

		inline const bool GetEnabledBindMeshes() const { return m_enabledBindMeshes; }
		inline const bool GetEnabledBindMaterials() const { return m_enabledBindMaterials; }

		inline bool GetPassCheckKeyword(std::string_view keyword) const { return m_passCheckKeywords.find(std::string(keyword)) != m_passCheckKeywords.end(); }
	};
}