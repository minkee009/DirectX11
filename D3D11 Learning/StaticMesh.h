#pragma once
#include "Mesh.h"
#include "Material.h"

namespace MyEngine
{
	class StaticMesh
	{
	private:
		std::vector<Mesh> m_subMesh;
		std::vector<UINT> m_matIdx;
	public:
		void SetSubMesh(std::vector<Mesh>&& subMesh) { m_subMesh = std::move(subMesh); }
	};
}