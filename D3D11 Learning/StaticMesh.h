#pragma once
#include "Mesh.h"
#include "Resource.h"

namespace MyEngine
{
	class StaticMesh : public Resource
	{
	private:
		std::vector<std::shared_ptr<Mesh>> m_subMeshes;
	protected:
		mutable BoundingBox m_bbox;
		bool m_isBBoxCalculated = false;
	public:
		StaticMesh();
		StaticMesh(StaticMesh&& other) noexcept;
		void SetSubMesh(std::vector<std::shared_ptr<Mesh>>&& subMesh) { m_subMeshes = std::move(subMesh); }

		void CalcBBox();

		inline const BoundingBox& GetBBox() { if (!m_isBBoxCalculated) { m_isBBoxCalculated = true; CalcBBox(); } return m_bbox; }
		inline std::vector<std::shared_ptr<Mesh>>& GetMeshes() { return m_subMeshes; }
	};
}