#include "BVH.h"
#include <algorithm>
#include <numeric>

void MyEngine::StaticBVH::Build(const std::vector<BoundingBox>& registry)
{
    if (registry.empty())
        return;

    Clear();
    m_nodes.reserve(registry.size() * 2);

    // 인덱스 배열 생성
    m_alignedIndices.resize(registry.size());
    std::iota(m_alignedIndices.begin(), m_alignedIndices.end(), 0);

    m_rootIdx = BuildNode(registry, 0, registry.size());
}

void MyEngine::StaticBVH::Clear()
{
    m_nodes.clear();
    m_alignedIndices.clear();
    m_rootIdx = 0;
}

void MyEngine::StaticBVH::Search(size_t nodeIdx, const std::vector<BoundingBox>& registry, const BoundingBox& query, std::vector<size_t>& out)
{
    if (m_nodes.empty() || nodeIdx >= m_nodes.size()) return;

    const Node& currentNode = m_nodes[nodeIdx];

    if (!currentNode.bound.Intersects(query)) return;

    if (currentNode.IsLeaf())
    {
        for (size_t i = 0; i < currentNode.objCount; ++i)
        {
            size_t sortedIdx = currentNode.firstObjectIdx + i;
            size_t objIdx = m_alignedIndices[sortedIdx];

            if (objIdx < registry.size() && registry[objIdx].Intersects(query))
                out.push_back(objIdx);
        }
    }
    else
    {
        Search(currentNode.left, registry, query, out);
        Search(currentNode.right, registry, query, out);
    }
}

void MyEngine::StaticBVH::Search(size_t nodeIdx, const std::vector<BoundingBox>& registry, const BoundingFrustum& query, std::vector<size_t>& out)
{
    if (m_nodes.empty() || nodeIdx >= m_nodes.size()) return;

    const Node& currentNode = m_nodes[nodeIdx];

    if (!currentNode.bound.Intersects(query)) return;

    if (currentNode.IsLeaf())
    {
        for (size_t i = 0; i < currentNode.objCount; ++i)
        {
            size_t sortedIdx = currentNode.firstObjectIdx + i;
            size_t objIdx = m_alignedIndices[sortedIdx];

            if (objIdx < registry.size() && registry[objIdx].Intersects(query))
                out.push_back(objIdx);
        }
    }
    else
    {
        Search(currentNode.left, registry, query, out);
        Search(currentNode.right, registry, query, out);
    }
}

size_t MyEngine::StaticBVH::BuildNode(const std::vector<BoundingBox>& registry, size_t startIdx, size_t registrySize)
{
    size_t nodeIdx = m_nodes.size();
    m_nodes.emplace_back();

    size_t endIdx = startIdx + registrySize;

    // 경계박스 계산
    BoundingBox bound = registry[m_alignedIndices[startIdx]];
    for (size_t i = startIdx + 1; i < endIdx; ++i)
    {
        BoundingBox temp;
        BoundingBox::CreateMerged(temp, bound, registry[m_alignedIndices[i]]);
        bound = temp;
    }

    // 오브젝트가 2개 이하인 경우 바로 저장
    if (registrySize <= 2)
    {
        m_nodes[nodeIdx].bound = bound;
        m_nodes[nodeIdx].firstObjectIdx = startIdx;
        m_nodes[nodeIdx].objCount = registrySize;
        return nodeIdx;
    }

    // 아닌 경우 긴 축을 기준으로 양분할
    auto extents = bound.Extents;

    int mid = startIdx + registrySize / 2;

    int axis = 0;
    if (extents.y > extents.x) axis = 1;
    if (extents.z > (axis == 0 ? extents.x : extents.y)) axis = 2;

    // 정렬
    std::nth_element(
        m_alignedIndices.begin() + startIdx,
        m_alignedIndices.begin() + mid,
        m_alignedIndices.begin() + endIdx,
        [&registry, axis](size_t a, size_t b) {
            switch (axis) {
            case 0: return registry[a].Center.x < registry[b].Center.x;
            case 1: return registry[a].Center.y < registry[b].Center.y;
            case 2: return registry[a].Center.z < registry[b].Center.z;
            default: return false;
            }
        }
    );

    // 좌우 자식 생성 (nodeIdx 재참조 전에)
    size_t leftChild = BuildNode(registry, startIdx, mid - startIdx);
    size_t rightChild = BuildNode(registry, mid, endIdx - mid);

    // 재귀 후 노드 설정 (벡터 재할당 대비)
    m_nodes[nodeIdx].bound = bound;
    m_nodes[nodeIdx].left = leftChild;
    m_nodes[nodeIdx].right = rightChild;
    m_nodes[nodeIdx].objCount = 0;

    return nodeIdx;
}


bool MyEngine::DynamicBVH::CheckSplitRule()
{
    return false;
}

void MyEngine::DynamicBVH::Refit()
{
}

void MyEngine::DynamicBVH::PartialRebuild(size_t parent_idx)
{
}

void MyEngine::DynamicBVH::Rotate(size_t idx)
{
    auto& nodeA = m_nodes[idx];
}

void MyEngine::DynamicBVH::Remove(size_t idx)
{
}

size_t MyEngine::DynamicBVH::Insert()
{
    return size_t();
}

void MyEngine::DynamicBVH::Build(const std::vector<BoundingBox>& registry)
{

}