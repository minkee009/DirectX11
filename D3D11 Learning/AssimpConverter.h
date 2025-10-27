#pragma once
#include <memory>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "RigidMeshRenderer.h"
#include "StaticMeshRenderer.h"
#include "SkinningMeshRenderer.h"

namespace MyEngine
{
	class AssimpConverter
	{
	private:
		static std::unique_ptr<Assimp::Importer> s_importer;
		static uint32_t s_importFlags;
		static ID3D11Device* s_pDevice;
		static ID3D11DeviceContext* s_pContext;

		enum class BoneType { None, RigidBone, SkinningBone };

		struct CorrectionNode
		{
			UINT meshIdx;
			const aiBone* pBone;
		};

		static void ProcessNode(std::vector<Mesh>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene);
		static void ProcessNode(int parentIndex, std::vector<RigidBone>& bones, std::vector<Mesh>& meshes, std::vector<UINT>& matIDX, std::vector<UINT>& boneIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string,UINT>& nodeNameToIndexMap);
		static void ProcessNode(int parentIndex, std::vector<SkinningBone>& bones, std::vector<Mesh>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string,UINT>& nodeNameToIndexMap, std::vector<CorrectionNode>& correctionMap);
		static Mesh ProcessMesh(aiMesh* pMesh, const aiScene* pScene);
		static Material ProcessMaterial(aiMaterial* pMat, const aiScene* pScene,const BoneType& boneType);

	public:
		static void Initialize(ID3D11DeviceContext* context);
		static void Release();
		static std::unique_ptr<RigidMeshRenderer> LoadRigidMeshRendererFromFile(std::string filePath);
		static std::unique_ptr<StaticMeshRenderer> LoadStaticMeshRendererFromFile(std::string filePath);
		static std::unique_ptr<SkinningMeshRenderer> LoadSkinningMeshRendererFromFile(std::string filePath);
	};

}