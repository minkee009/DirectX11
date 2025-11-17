#pragma once
#include "Mesh.h"

namespace MyEngine
{
	class StaticMesh
	{
	private:
		std::shared_ptr<std::vector<Mesh>> m_pSubMeshes;
	protected:
		mutable BoundingBox m_bbox;
		bool isBBoxCalculated = false;
	public:
		void SetSubMesh(std::shared_ptr<std::vector<Mesh>> subMesh) { m_pSubMeshes = subMesh; }

		void CalcBBox();

		inline const BoundingBox& GetBBox() { if (!isBBoxCalculated) { isBBoxCalculated = true; CalcBBox(); } return m_bbox; }
		inline std::vector<Mesh>& GetMeshes() { return *m_pSubMeshes; }
	};
}