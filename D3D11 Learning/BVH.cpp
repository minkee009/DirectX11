#include "BVH.h"
#include <algorithm>
#include <numeric>

void MyEngine::BVH::Build(const std::vector<BoundingBox*>& registry)
{
	if (registry.empty())
		return;

	Clear();
	m_nodes.reserve(registry.size() * 2);

	// 인덱스 배열 생성
	std::vector<size_t> indices(registry.size());
	std::iota(indices.begin(), indices.end(), 0);

	m_rootIdx = BuildNode(registry, indices, 0, registry.size());
}

void MyEngine::BVH::Clear()
{
    m_nodes.clear();
    m_rootIdx = 0;
}

void MyEngine::BVH::Search(size_t nodeIdx, const std::vector<BoundingBox*>& registry, const BoundingBox& query, std::vector<size_t>& out)
{
    if (m_nodes.empty() || nodeIdx >= m_nodes.size()) return;

    const Node& currentNode = m_nodes[nodeIdx];

    if (!currentNode.bound.Intersects(query)) return;

    if (currentNode.IsLeaf())
    {
        for (size_t i = 0; i < currentNode.objCount; ++i)
        {
            size_t objIdx = currentNode.firstObject + i;
            if (objIdx < registry.size() && registry[objIdx]->Intersects(query))
                out.push_back(objIdx);
        }
    }
    else
    {
        Search(currentNode.left, registry, query, out);
        Search(currentNode.right, registry, query, out);
    }
}

void MyEngine::BVH::Search(size_t nodeIdx, const std::vector<BoundingBox*>& registry, const BoundingFrustum& query, std::vector<size_t>& out)
{
    if (m_nodes.empty() || nodeIdx >= m_nodes.size()) return;

    const Node& currentNode = m_nodes[nodeIdx];

    if (!currentNode.bound.Intersects(query)) return;

    if (currentNode.IsLeaf())
    {
        for (size_t i = 0; i < currentNode.objCount; ++i)
        {
            size_t objIdx = currentNode.firstObject + i;
            if (objIdx < registry.size() && registry[objIdx]->Intersects(query))
                out.push_back(objIdx);
        }
    }
    else
    {
        Search(currentNode.left, registry, query, out);
        Search(currentNode.right, registry, query, out);
    }
}

size_t MyEngine::BVH::BuildNode(const std::vector<BoundingBox*>& registry, std::vector<size_t>& indices, size_t startIdx, size_t registrySize)
{
	return size_t();
}
