#pragma once
#include <string>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include "Transform.h"


using namespace DirectX;

namespace MyEngine
{
	struct MeshVertex
	{
		XMFLOAT3 pos;
		XMFLOAT3 nor;
		XMFLOAT2 uv;
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();
		std::shared_ptr<Transform> pTransform;
		static std::unique_ptr<Mesh> CreateFromFile(std::wstring path);
		std::unique_ptr<std::vector<MeshVertex> > pMeshData;
	};
}