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
		bool m_enabledBindMeshes;
		bool m_enabledBindMaterials;
		std::unordered_map<UINT, std::unordered_set<UINT>> m_passExcludedMeshes; // < render pass, { dont draw mesh idx } >
		std::unordered_map<UINT, ID3D11VertexShader*> m_passForceChangeVS;
		//std::unordered_map<UINT, ID3D11PixelShader*> m_passForceChangePS;

	protected:
		std::vector<Material> m_materials;
		inline const std::unordered_map<UINT, std::unordered_set<UINT>>& GetPassExcludedMeshes() const { return m_passExcludedMeshes; };
		inline const std::unordered_map<UINT, ID3D11VertexShader*>& GetPassForceChangeVS() const { return m_passForceChangeVS; };
		inline const UINT& GetRenderPassNum() const { return m_renderPassNum; }
	public:
		virtual void Draw(ID3D11DeviceContext* context) = 0;
		virtual const BoundingBox& GetBBox() = 0;

		void SetPassExcludedMeshes(UINT renderPassNum, std::initializer_list<UINT> meshes);
		void SetPassForceChangeVS(UINT renderPassNum, ID3D11VertexShader* VS);

		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		inline void SetRenderPassNum(UINT renderPassNum) { m_renderPassNum = renderPassNum; }

		inline void ClearPassExcludedMeshes() { m_passExcludedMeshes.clear(); }
		inline void ClearPassForceChangeVS() { m_passForceChangeVS.clear(); }

		inline void SetEnabledBindMeshes(bool enabled) { m_enabledBindMeshes = enabled; }
		inline void SetEnabledBindMaterials(bool enabled) { m_enabledBindMaterials = enabled; }

		inline const bool GetEnabledBindMeshes() const { return m_enabledBindMeshes; }
		inline const bool GetEnabledBindMaterials() const { return m_enabledBindMaterials; }
	};
}