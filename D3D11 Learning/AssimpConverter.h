#pragma once
#include <memory>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "RigidMeshRenderer.h"
#include "StaticMeshRenderer.h"

namespace MyEngine
{
	//class FBXSceneGraph 
	//{
	//private:
	//	friend class AssimpConverter;
	//	std::vector<Mesh> m_meshes;
	//	std::vector<UINT> m_matIdxes;
	//	std::vector<Material> m_materials;
	//public:
	//	void Draw(ID3D11DeviceContext* context);
	//};

	class AssimpConverter
	{
	private:
		static std::unique_ptr<Assimp::Importer> s_importer;
		static uint32_t s_importFlags;
		static ID3D11Device* s_pDevice;
		static ID3D11DeviceContext* s_pContext;

		static void ProcessNode(std::vector<Mesh>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene);
		static void ProcessNode(int parentIndex, std::vector<RigidBone>& bones, std::vector<Mesh>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string,UINT>& nodeNameToIndexMap);
		static Mesh ProcessMesh(aiMesh* pMesh, const aiScene* pScene);
		static Material ProcessMaterial(aiMaterial* pMat, const aiScene* pScene);

	public:
		static void Initialize(ID3D11DeviceContext* context);
		static void Release();
		static std::unique_ptr<RigidMeshRenderer> LoadRigidMeshRendererFromFile(std::string filePath);
		static std::unique_ptr<StaticMeshRenderer> LoadStaticMeshRendererFromFile(std::string filePath);
	};

}