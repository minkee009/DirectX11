#include "StaticMesh.h"

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
