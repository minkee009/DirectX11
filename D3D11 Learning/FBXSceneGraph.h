#pragma once
#include <memory>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#include "Mesh.h"
#include "Material.h"

namespace MyEngine
{
	class FBXSceneGraph 
	{
	private:
		friend class AssimpConverter;
		std::vector<Mesh> m_meshes;
		std::vector<Material> m_materials;
	public:
		void Draw(ID3D11DeviceContext* context);
	};

	class AssimpConverter
	{
	private:
		static std::unique_ptr<Assimp::Importer> s_importer;
		static uint32_t s_importFlags;
		static ID3D11Device* s_pDevice;

		static void ProcessNode(std::vector<Mesh>& meshes, aiNode* pNode, const aiScene* pScene);
		static Mesh ProcessMesh(std::vector<Mesh>& meshes, aiMesh* pMesh, const aiScene* pScene);

	public:
		static void Initialize(ID3D11Device* device);
		static void Release();
		static std::unique_ptr<FBXSceneGraph> LoadFromFile(std::string filePath);
	};

}