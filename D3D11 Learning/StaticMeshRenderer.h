#pragma once
#include <memory>
#include "StaticMesh.h"
#include "Material.h"

namespace DebugStatusUI
{
	class StaticMeshRenderer
	{
	public:
		static bool limitDrawOption;
		static int meshNum;
	};
}

namespace MyEngine
{
	class StaticMeshRenderer
	{
	private:
		StaticMesh m_staticMesh;
		std::vector<Material> m_materials;
		std::unordered_set<UINT> m_dontDrawMeshNums;
	public:
		inline void SetMesh(StaticMesh&& mesh) { m_staticMesh = std::move(mesh); }
		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		void Draw(ID3D11DeviceContext* context, bool bindMesh = true, bool bindMaterial = true, std::initializer_list<UINT> dontDrawMeshNums = {});
	};
}

