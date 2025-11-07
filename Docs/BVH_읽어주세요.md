# BVH

- BVH.h

```c++
#pragma once
#include <directxtk/SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyEngine
{
	class BVH
	{
	public:
		struct Node
		{
			BoundingBox bound;
			size_t left = 0;
			size_t right = 0;
			size_t firstObjectIdx = 0;
			size_t objCount = 0;

			bool IsLeaf() const { return objCount > 0; }
		};
		/// <summary>
		/// BVH를 생성합니다. 내부로직에서 BoundingBox배열의 인덱스를 이용한 트리노드를 생성합니다.
		/// </summary>
		/// <param name="registry"></param>
		void Build(const std::vector<BoundingBox>& registry);
		void Clear();

		/// <summary>
		/// BVH 트리 내부의 노드를 순회해 겹치는 오브젝트를 반환합니다.
		/// BVH 트리 빌드 시 사용하였던 BoundingBox배열 레지스트리가 필요합니다.
		/// </summary>
		/// <param name="nodeIdx"> - 순회를 시작할 노드의 위치입니다. 특별한 경우가 아니면 최상단 노드를 넣는 것을 추천합니다.</param>
		/// <param name="registry"> - 트리가 참조중인 BoundingBox배열입니다. 빌드함수를 호출할 때 사용했던 배열을 넣어야합니다.</param>
		/// <param name="query"> - 기준 경계 볼륨입니다, 이 기준으로 레지스트리 내부에 겹치는 BoundingBox를 골라냅니다. </param>
		/// <param name="out"> - 겹쳐진 BoundingBox의 레지스트리 인덱스 모음입니다. </param>
		void Search(size_t nodeIdx, 
                    const std::vector<BoundingBox>& registry, 
                    const BoundingBox& query, 
                    std::vector<size_t>& out);

		void Search(size_t nodeIdx, 
                    const std::vector<BoundingBox>& registry, 
                    const BoundingFrustum& query, 
                    std::vector<size_t>& out);

		inline const size_t& GetRootIdx() const { return m_rootIdx; }

		inline const size_t& GetMappedIdx(size_t alignedIdx) const { if (alignedIdx > 0 && alignedIdx < m_alignedIndices.size()) return m_alignedIndices[alignedIdx]; return 0; }
	private:
		size_t m_rootIdx = 0;
		std::vector<Node> m_nodes;
		std::vector<size_t> m_alignedIndices;

		size_t BuildNode(const std::vector<BoundingBox>& registry, size_t startIdx, size_t registrySize);
	};
}
```



- BVH.cpp

```c++
#include "BVH.h"
#include <algorithm>
#include <numeric>

void MyEngine::BVH::Build(const std::vector<BoundingBox>& registry)
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

void MyEngine::BVH::Clear()
{
    m_nodes.clear();
    m_alignedIndices.clear();
    m_rootIdx = 0;
}

void MyEngine::BVH::Search(size_t nodeIdx, const std::vector<BoundingBox>& registry, const BoundingBox& query, std::vector<size_t>& out)
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

void MyEngine::BVH::Search(size_t nodeIdx, const std::vector<BoundingBox>& registry, const BoundingFrustum& query, std::vector<size_t>& out)
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

size_t MyEngine::BVH::BuildNode(const std::vector<BoundingBox>& registry, size_t startIdx, size_t registrySize)
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

	// 좌우 자식 생성
	size_t leftChild = BuildNode(registry, startIdx, mid - startIdx);
	size_t rightChild = BuildNode(registry, mid, endIdx - mid);

	// 재귀 후 노드 설정
	m_nodes[nodeIdx].bound = bound;
	m_nodes[nodeIdx].left = leftChild;
	m_nodes[nodeIdx].right = rightChild;
	m_nodes[nodeIdx].objCount = 0;

	return nodeIdx;
}
```



# 작동 방식

- 씬을 처음 초기화 할 때 BVH.build()를 호출하여 모든 오브젝트의 처음위치로 BVH를 한번만 생성합니다.
- 매 프레임마다 m_usingBVH 변수를 읽어 BVH를 사용하는 경우에만 BVH.Search()를 통해 카메라의 절두체를 이용하여 컬링한 결과를 반영한 드로우콜을 생성합니다.
  - BVH를 사용하지 않는 경우 모든 오브젝트에 대해 드로우콜을 생성합니다.
- 좌측 상단의 "디버그 드로잉" ImGui창으로 FPS와 드로우콜이 몇 개인지 파악할 수 있습니다.



# 데모 영상

- 데모영상에는 카메라 절두체로 컬링하는 모습을 제 3자 시점으로 관찰합니다.

  https://youtu.be/5RWWsGrxJR8