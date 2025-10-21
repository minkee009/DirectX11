#pragma once
#include "Mesh.h"

namespace MyEngine
{
	class StaticMesh
	{
	private:
		std::vector<Mesh> m_subMesh;
		std::vector<UINT> m_matIdx;
	public:
		void SetSubMesh(std::vector<Mesh>&& subMesh) { m_subMesh = std::move(subMesh); }
		void SetMatIdx(std::vector<UINT>&& matIdx) { m_matIdx = std::move(matIdx); }

		inline const std::vector<Mesh>& GetMeshes() const { return m_subMesh; }
		inline const std::vector<UINT>& GetMaterialIndices() const { return m_matIdx; }
	};
}