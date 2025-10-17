#pragma once
#include <memory>
#include "StaticMesh.h"

namespace MyEngine
{
	class StaticMeshRenderer
	{
	private:
		StaticMesh m_staticMesh;
		std::vector<Material> m_materials;
	public:
		inline void SetMesh(StaticMesh&& mesh) { m_staticMesh = std::move(mesh); }
		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		void Draw(ID3D11DeviceContext* context);
	};
}

