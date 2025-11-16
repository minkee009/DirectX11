#pragma once
#include "Mesh.h"

namespace MyEngine
{
	class StaticMesh
	{
	private:
		std::vector<Mesh> m_subMesh;
	protected:
		mutable BoundingBox m_bbox;
		bool isBBoxCalculated = false;
	public:
		void SetSubMesh(std::vector<Mesh>&& subMesh) { m_subMesh = std::move(subMesh); }

		virtual void CalcBBox();

		inline const BoundingBox& GetBBox() { if (!isBBoxCalculated) { isBBoxCalculated = true; CalcBBox(); } return m_bbox; }
		inline std::vector<Mesh>& GetMeshes() { return m_subMesh; }
	};
}