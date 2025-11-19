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
	class AssimpModel : public Resource
	{
	public:
		const aiScene* pScene = nullptr;
		Assimp::Importer importer;

		bool LoadFromFile(const std::string& path, unsigned int flags)
		{
			pScene = importer.ReadFile(path, flags);
			return pScene != nullptr;
		}
	};

	class AssimpConverter
	{
	private:
		static uint32_t s_importFlags;
		static ID3D11Device* s_pDevice;
		static ID3D11DeviceContext* s_pContext;

		static std::vector<std::shared_ptr<AssimpModel>> s_loadedModels;

		enum class BoneType { None, RigidBone, SkinningBone };

		struct CorrectionNode
		{
			UINT meshIdx;
			const aiBone* pBone;
		};

		static std::unordered_set<std::string> CollectUsedBoneNames(const aiScene* pScene);
		static void CollectBoneHierarchy(aiNode* pNode, const std::unordered_set<std::string>& usedBones, std::unordered_set<std::string>& boneHierarchy);

		static void ProcessNode(std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene);
		static void ProcessNode(int parentIndex, std::vector<RigidBone>& bones, std::vector<RigidBonePose>& bonePoses, std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<UINT>& matIDX, std::vector<UINT>& boneIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string, UINT>& nodeNameToIndexMap);
		static void ProcessNode(int parentIndex, std::vector<SkinningBone>& bones, std::vector<SkinningBonePose>& bonePoses,std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string, UINT>& nodeNameToIndexMap, std::vector<CorrectionNode>& correctionMap, const std::unordered_set<std::string>& boneHierarchy);
		static void ProcessMesh(aiMesh* pMesh, std::shared_ptr<Mesh> resourceMesh, const aiScene* pScene);
		static void ProcessMaterial(aiMaterial* pMat, std::shared_ptr<Material> resourceMat, const aiScene* pScene, const BoneType& boneType);
	public:
		enum class LoadMaterialType { BlinnPhong, BlinnPhongToon };

		static void Initialize(ID3D11DeviceContext* context);
		static void Release();
		static std::unique_ptr<StaticMeshRenderer> LoadStaticMeshRendererFromFile(std::string filePath);
		static std::unique_ptr<RigidMeshRenderer> LoadRigidMeshRendererFromFile(std::string filePath);
		static std::unique_ptr<SkinningMeshRenderer> LoadSkinningMeshRendererFromFile(std::string filePath);
		static void SetLoadMaterialType(LoadMaterialType type);

	private:
		static LoadMaterialType s_materialType;
	};

}