#include "StaticMesh.h"

MyEngine::StaticMesh::StaticMesh()
	: m_pSubMeshes(nullptr)
	, m_bbox()
	, m_isBBoxCalculated(false)
{
}

MyEngine::StaticMesh::StaticMesh(StaticMesh&& other) noexcept
	: m_pSubMeshes(std::move(other.m_pSubMeshes))
	, m_bbox(std::move(other.m_bbox))
	, m_isBBoxCalculated(std::move(other.m_isBBoxCalculated))
{
}

void MyEngine::StaticMesh::CalcBBox()
{
	auto& meshes = GetMeshes();
	if (meshes.empty())
		return;
	m_bbox = meshes[0].GetBBox();

	for (size_t i = 1; i < meshes.size(); i++)
	{
		BoundingBox::CreateMerged(m_bbox, m_bbox, meshes[i].GetBBox());
	}
}
