#pragma once
#include "Mesh.h"
#include "AABB.h"

namespace MyEngine
{
	class StaticMesh
	{
	private:
		std::vector<Mesh> m_subMesh;
		std::vector<UINT> m_matIdx;
	protected:
		AABB m_aabb;
	public:
		void SetSubMesh(std::vector<Mesh>&& subMesh) { m_subMesh = std::move(subMesh); }
		void SetMatIdx(std::vector<UINT>&& matIdx) { m_matIdx = std::move(matIdx); }

		virtual void CalcAABB();

		inline const AABB& GetAABB() const { return m_aabb; }

		inline std::vector<Mesh>& GetMeshes() { return m_subMesh; }
		inline std::vector<UINT>& GetMaterialIndices() { return m_matIdx; }
	};
}