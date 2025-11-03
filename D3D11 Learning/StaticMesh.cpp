#include "StaticMesh.h"

void MyEngine::StaticMesh::CalcAABB()
{
	auto& meshes = GetMeshes();
	if (meshes.empty())
		return;

	float minX, minY, minZ;
	float maxX, maxY, maxZ;
	minX = minY = minZ = FLT_MAX;
	maxX = maxY = maxZ = -FLT_MAX;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const auto& meshAABB = meshes[i].GetAABB();

		if (meshAABB.min.x < minX) minX = meshAABB.min.x;
		if (meshAABB.min.y < minY) minY = meshAABB.min.y;
		if (meshAABB.min.z < minZ) minZ = meshAABB.min.z;
		if (meshAABB.max.x > maxX) maxX = meshAABB.max.x;
		if (meshAABB.max.y > maxY) maxY = meshAABB.max.y;
		if (meshAABB.max.z > maxZ) maxZ = meshAABB.max.z;
	}

	m_aabb.min = { minX, minY, minZ };
	m_aabb.max = { maxX, maxY, maxZ };
}
