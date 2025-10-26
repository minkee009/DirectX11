#pragma once
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;

namespace MyEngine
{
	struct VertexType
	{
		Vector3 position;
		Vector3 normal;
		Vector3 tangent;
		Vector2 uv;
		UINT boneIndices[4];
		float boneWeights[4];
	};
}


