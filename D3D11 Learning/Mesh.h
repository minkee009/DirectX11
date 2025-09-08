#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "Transform.h"
#include <string>

using namespace DirectX;

namespace MyEngine
{
	struct MeshVertex
	{
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();
		std::shared_ptr<Transform> transform;

		static std::unique_ptr<Mesh> CreateFromFile(std::wstring path);
	};
}