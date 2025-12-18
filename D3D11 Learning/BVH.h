#pragma once
#include <directxtk/SimpleMath.h>
#include "Transform.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyEngine
{
	class StaticBVH
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
		/// StaticBVH 트리 내부의 노드를 순회해 겹치는 오브젝트를 반환합니다.
		/// StaticBVH 트리 빌드 시 사용하였던 BoundingBox배열 레지스트리가 필요합니다.
		/// </summary>
		/// <param name="nodeIdx"> - 순회를 시작할 노드의 위치입니다. 특별한 경우가 아니면 최상단 노드를 넣는 것을 추천합니다.</param>
		/// <param name="registry"> - 트리가 참조중인 BoundingBox배열입니다. 빌드함수를 호출할 때 사용했던 배열을 넣어야합니다.</param>
		/// <param name="query"> - 기준 경계 볼륨입니다, 이 기준으로 레지스트리 내부에 겹치는 BoundingBox를 골라냅니다. </param>
		/// <param name="out"> - 겹쳐진 BoundingBox의 레지스트리 인덱스 모음입니다. </param>
		void Search(size_t nodeIdx, const std::vector<BoundingBox>& registry, const BoundingBox& query, std::vector<size_t>& out);

		void Search(size_t nodeIdx, const std::vector<BoundingBox>& registry, const BoundingFrustum& query, std::vector<size_t>& out);

		inline const size_t& GetRootIdx() const { return m_rootIdx; }

		inline const size_t& GetMappedIdx(size_t alignedIdx) const { if (alignedIdx > 0 && alignedIdx < m_alignedIndices.size()) return m_alignedIndices[alignedIdx]; return 0; }

		//=== debug 코드 ===//
		static inline BoundingBox MakeTransformedBBox(const Transform& t, const BoundingBox& bbox)
		{
			BoundingBox transformedBBox{};

			bbox.Transform(transformedBBox, t.GetWorldMatrix());

			return transformedBBox;
		}

	private:
		size_t m_rootIdx = 0;
		std::vector<Node> m_nodes;
		std::vector<size_t> m_alignedIndices;

		size_t BuildNode(const std::vector<BoundingBox>& registry, size_t startIdx, size_t registrySize);
	};

	class DynamicBVH
	{
	private:
		static constexpr size_t INVALID_IDX = size_t(-1);

		// 포인터 형식의 노드
		struct Node
		{
			BoundingBox bound;
			size_t parent = INVALID_IDX;
			size_t left = INVALID_IDX;
			size_t right = INVALID_IDX;
			size_t objectIdx = INVALID_IDX;

			bool IsLeaf() const { return objectIdx != INVALID_IDX; }
		};

		std::vector<Node> m_nodes;

		float m_bboxMargin = 0.1f;

		void Rotate(size_t idx);

	public:
		// 최초 빌드용
		void Build(const std::vector<BoundingBox>& registry);
	};
}