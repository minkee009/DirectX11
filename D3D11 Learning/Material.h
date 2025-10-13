#pragma once
#include "VertexType.h"
#include <directxtk/SimpleMath.h>
#include <vector>
#include <assimp/mesh.h>
#include <string>

using namespace DirectX::SimpleMath;

namespace MyEngine
{
	class Material
	{
	public:
		std::string name;
		Color ambient;
		Color diffuse;
		Color specular;
		Color emissive;
		std::string diffuseFile;
		std::string specularFile;
		std::string normalFile;
	};
}