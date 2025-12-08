#include "AssimpConverter.h"
#include <queue>
#include <stdexcept>
#include <filesystem>

uint32_t MyEngine::AssimpConverter::s_importFlags = 0;
ID3D11Device* MyEngine::AssimpConverter::s_pDevice = nullptr;
ID3D11DeviceContext* MyEngine::AssimpConverter::s_pContext = nullptr;
std::vector<std::shared_ptr<MyEngine::AssimpModel>> MyEngine::AssimpConverter::s_loadedModels;

namespace fs = std::filesystem;
using namespace MyEngine::D3DCTX;

#include "ShaderManager.h"
#include "TextureManager.h"
#include "ResourceManager.h"

// ======== 리소스 매니저와의 연계 , 꼭 읽어 주세요 ========
// 
// 
// 1. 리소스 키는 다음과 같은 규칙으로 생성
//  -> aiScene 이름 + ai클래스의 mName 
// 
// 2. 하위 리소스의 경우 파일 경로로 키 생성 
//  -> 내부로직에서 embedded리소스였을 경우에만 [클래스 이름]- + 파일경로 형태로 키 생성 
//
//
// ======================================================

std::unordered_set<std::string> MyEngine::AssimpConverter::CollectUsedBoneNames(const aiScene* pScene)
{
    std::unordered_set<std::string> usedBones;

    // 모든 메시를 순회하며 실제 스키닝에 사용되는 본 이름 수집
    for (UINT i = 0; i < pScene->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[i];
        if (pMesh->HasBones())
        {
            for (UINT j = 0; j < pMesh->mNumBones; j++)
            {
                usedBones.insert(pMesh->mBones[j]->mName.C_Str());
            }
        }
    }

    return usedBones;
}

#include <iostream>

void MyEngine::AssimpConverter::CollectBoneHierarchy(aiNode* pNode, const std::unordered_set<std::string>& usedBones, std::unordered_set<std::string>& boneHierarchy)
{
    std::string nodeName = pNode->mName.C_Str();
    bool isUsedBone = usedBones.find(nodeName) != usedBones.end();
    bool hasChildBone = false;

    // 자식들을 먼저 확인
    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        CollectBoneHierarchy(pNode->mChildren[i], usedBones, boneHierarchy);

        std::string childName = pNode->mChildren[i]->mName.C_Str();
        if (boneHierarchy.find(childName) != boneHierarchy.end())
        {
            hasChildBone = true;
        }
    }

    // 이 노드가 본이거나, 자식 중에 본이 있으면 계층에 포함
    if (isUsedBone || hasChildBone)
    {
        boneHierarchy.insert(nodeName);
    }
}

void MyEngine::AssimpConverter::ProcessNode(std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene)
{
    //메쉬 정보 처리
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        std::shared_ptr<Mesh> mesh = nullptr;
        std::string meshName = "[SubMesh]-" + std::to_string(meshes.size()) + std::string{pMesh->mName.C_Str()};
        if (ResourceManager::Get()->Containskey(meshName))
        {
            mesh = ResourceManager::Get()->Load<Mesh>(meshName);
        }
        else
        {
            mesh = ResourceManager::Get()->Load<Mesh>(meshName);
            ProcessMesh(pMesh, mesh, pScene);
        }

        meshes.push_back(mesh);
        matIDX.push_back(pMesh->mMaterialIndex);
    }

    //자식 노드 처리
    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        ProcessNode(meshes, matIDX, pNode->mChildren[i], pScene);
    }
}

void MyEngine::AssimpConverter::ProcessNode(int parentIndex, std::vector<RigidBone>& bones, std::vector<RigidBonePose>& bonePoses, std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<UINT>& matIDX, std::vector<UINT>& boneIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string, UINT>& nodeNameToIndexMap)
{
    //메쉬 정보 처리
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        std::shared_ptr<Mesh> mesh = nullptr;
        std::string meshName = "[SubMesh]-" + std::to_string(meshes.size()) + std::string{ pMesh->mName.C_Str() };
        if (ResourceManager::Get()->Containskey(meshName))
        {
            mesh = ResourceManager::Get()->Load<Mesh>(meshName);
        }
        else
        {
            mesh = ResourceManager::Get()->Load<Mesh>(meshName);
            ProcessMesh(pMesh, mesh, pScene);
        }

        meshes.push_back(mesh);
        matIDX.push_back(pMesh->mMaterialIndex);
        boneIDX.push_back(static_cast<int>(bones.size()));
        //std::cout << boneIDX.size() << " : " << pMesh->mName.C_Str() << "[ parent : " << (currentDepth - 1) << " ]" << std::endl;
    }

    RigidBone bone;

    auto& matrix = pNode->mTransformation;
    bone.index = static_cast<int>(bones.size());
    bone.parentIndex = parentIndex;

    RigidBonePose bonePose;
    bonePose.local = Matrix(
        matrix.a1, matrix.a2, matrix.a3, matrix.a4,
        matrix.b1, matrix.b2, matrix.b3, matrix.b4,
        matrix.c1, matrix.c2, matrix.c3, matrix.c4,
        matrix.d1, matrix.d2, matrix.d3, matrix.d4
    );
    
    bonePoses.emplace_back(bonePose);
    bones.emplace_back(bone);
    nodeNameToIndexMap.insert({ pNode->mName.C_Str(),bone.index });

    //자식 노드 처리
    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        ProcessNode(bone.index, bones, bonePoses, meshes, matIDX, boneIDX, pNode->mChildren[i], pScene, nodeNameToIndexMap);
    }
}

void MyEngine::AssimpConverter::ProcessNode(int parentIndex, std::vector<SkinningBone>& bones, std::vector<SkinningBonePose>& bonePoses, std::vector<std::shared_ptr<Mesh>>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string, UINT>& nodeNameToIndexMap, std::vector<CorrectionNode>& correctionMap, const std::unordered_set<std::string>& boneHierarchy)
{
    std::string nodeName = pNode->mName.C_Str();
    bool isInBoneHierarchy = boneHierarchy.find(nodeName) != boneHierarchy.end();

    int currentBoneIndex = -1;

    // 본 계층에 포함된 노드만 본으로 생성
    if (isInBoneHierarchy)
    {
        SkinningBone bone;
        auto& matrix = pNode->mTransformation;
        bone.index = static_cast<int>(bones.size());
        bone.parentIndex = parentIndex;

        SkinningBonePose bonePose;
        bonePose.local = Matrix(
            matrix.a1, matrix.a2, matrix.a3, matrix.a4,
            matrix.b1, matrix.b2, matrix.b3, matrix.b4,
            matrix.c1, matrix.c2, matrix.c3, matrix.c4,
            matrix.d1, matrix.d2, matrix.d3, matrix.d4
        );

        bonePoses.emplace_back(bonePose);
        bones.emplace_back(bone);
        nodeNameToIndexMap.insert({ nodeName, bone.index });
        currentBoneIndex = bone.index;
    }

    //메쉬 정보 처리
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        std::shared_ptr<Mesh> mesh = nullptr;
        std::string meshName = "[SubMesh]-" + std::to_string(meshes.size()) + std::string{pMesh->mName.C_Str()};
        if (ResourceManager::Get()->Containskey(meshName))
        {
            mesh = ResourceManager::Get()->Load<Mesh>(meshName);
        }
        else
        {
            mesh = ResourceManager::Get()->Load<Mesh>(meshName);
            ProcessMesh(pMesh, mesh, pScene);
        }

        meshes.push_back(mesh);
        matIDX.push_back(pMesh->mMaterialIndex);

        if (pMesh->HasBones())
        {
            UINT meshIdx = static_cast<UINT>(meshes.size()) - 1;
            for (UINT j = 0; j < pMesh->mNumBones; j++)
            {
                auto& aiBone = pMesh->mBones[j];
                correctionMap.push_back({ meshIdx,aiBone });
            }
        }
    }

    //자식 노드 처리
    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        // 부모 인덱스는 현재 노드가 본인 경우만 전달
        int childParentIndex = isInBoneHierarchy ? currentBoneIndex : parentIndex;
        ProcessNode(childParentIndex, bones, bonePoses, meshes, matIDX,
            pNode->mChildren[i], pScene, nodeNameToIndexMap,
            correctionMap, boneHierarchy);
    }
}


void MyEngine::AssimpConverter::ProcessMesh(aiMesh* pMesh, std::shared_ptr<Mesh> resourceMesh, const aiScene* pScene)
{
    std::vector<DefaultVertex> vertices;
    std::vector<UINT> indices;

    XMVECTOR minPt = { pMesh->mVertices[0].x, pMesh->mVertices[0].y, pMesh->mVertices[0].z };
    XMVECTOR maxPt = { pMesh->mVertices[0].x, pMesh->mVertices[0].y, pMesh->mVertices[0].z };

    for (UINT i = 0; i < pMesh->mNumVertices; i++)
    {
        DefaultVertex vertex;

        vertex.position = { pMesh->mVertices[i].x,pMesh->mVertices[i].y,pMesh->mVertices[i].z };

        XMVECTOR currentPoint = { pMesh->mVertices[i].x, pMesh->mVertices[i].y, pMesh->mVertices[i].z };

        minPt = XMVectorMin(minPt, currentPoint);
        maxPt = XMVectorMax(maxPt, currentPoint);

        vertex.normal = { pMesh->mNormals[i].x,pMesh->mNormals[i].y,pMesh->mNormals[i].z };
        vertex.tangent = { pMesh->mTangents[i].x, pMesh->mTangents[i].y ,pMesh->mTangents[i].z };
        if (pMesh->mTextureCoords[0])
        {
            vertex.uv = { (float)pMesh->mTextureCoords[0][i].x, (float)pMesh->mTextureCoords[0][i].y };
        }
        vertices.push_back(vertex);
    }

    for (UINT i = 0; i < pMesh->mNumFaces; i++) {
        aiFace face = pMesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    BoundingBox bbox;
    BoundingBox::CreateFromPoints(bbox, minPt, maxPt);

    resourceMesh->SetVertices(std::move(vertices));
    resourceMesh->SetIndices(std::move(indices));
    resourceMesh->SetBBox(std::move(bbox));
}

void MyEngine::AssimpConverter::ProcessMaterial(aiMaterial* pMat, std::shared_ptr<Material> resourceMat, const aiScene* pScene, const BoneType& boneType)
{
    resourceMat->SetName(pMat->GetName().C_Str());

    switch (boneType)
    {
    case BoneType::None:
        resourceMat->InitVertexShader(ShaderManager::Get()->GetCommonVertexShader());
        break;
    case BoneType::RigidBone:
        resourceMat->InitVertexShader(ShaderManager::Get()->GetCommonVertexShader_RigidBone());
        break;
    case BoneType::SkinningBone:
        resourceMat->InitVertexShader(ShaderManager::Get()->GetCommonVertexShader_SkinningBone());
        break;
    }

    switch (s_materialType)
    {
    case LoadMaterialType::BlinnPhong:
        resourceMat->InitPixelShader(ShaderManager::Get()->GetBlinnPhongPixelShader());
        break;
    case LoadMaterialType::BlinnPhongToon:
        resourceMat->InitPixelShader(ShaderManager::Get()->GetBlinnPhongToonPixelShader());
        break;
    case LoadMaterialType::BRDF:
        resourceMat->InitPixelShader(ShaderManager::Get()->GetBRDFPixelShader());
        break;
    }

    // ---- Texture 매핑 ---- //
    struct TexTypeMap {
        aiTextureType aiType;
        TextureType myType;
        UINT slot;
    };

    const std::vector<TexTypeMap> textureTypes = {
        { aiTextureType_DIFFUSE, TextureType::Diffuse, 0 },
        { aiTextureType_SPECULAR, TextureType::Specular, 3 },
        { aiTextureType_NORMALS, TextureType::Normal, 2 },
        { aiTextureType_EMISSIVE, TextureType::Emissive, 4 },
        //{ aiTextureType_HEIGHT, TextureType::Height, 4 },
        //{ aiTextureType_AMBIENT_OCCLUSION, TextureType::AmbientOcclusion, 5 },
        { aiTextureType_SHININESS, TextureType::Roughness, 6 },
        { aiTextureType_METALNESS, TextureType::Metalness, 7 },
    };

    static fs::path base_directory = std::filesystem::current_path() / "Resources/Textures";

	std::vector<aiTextureType> usedTextureTypes;

	for (int i = 0; i < pMat->mNumProperties; ++i)
	{
		aiMaterialProperty* prop = pMat->mProperties[i];

        if (prop->mSemantic)
        {
            auto semanticType = (aiTextureType)prop->mSemantic;
			usedTextureTypes.push_back(semanticType);
        }
	}

    for (const auto& [aiType, myType, slot] : textureTypes)
    {
        const UINT count = pMat->GetTextureCount(aiType);

        for (UINT i = 0; i < count; ++i)
        {
            aiString path;
            if (pMat->GetTexture(aiType, i, &path) == AI_SUCCESS)
            {
                std::string path_str = path.C_Str();

                bool use_sRGB = false; /*(myType == TextureType::Diffuse || myType == TextureType::Emissive);*/

                if (path_str.length() > 0 && path_str[0] == '*')
                {
                    size_t textureIndex = std::stoul(path_str.substr(1));
                    if (textureIndex < pScene->mNumTextures)
                    {
                        const aiTexture* embeddedTexture = pScene->mTextures[textureIndex];

                        const uint8_t* data = reinterpret_cast<const uint8_t*>(embeddedTexture->pcData);
                        size_t size = embeddedTexture->mWidth;

                        std::wstring formatExt = L"";
                        if (embeddedTexture->achFormatHint[0] != '\0')
                        {
                            std::string hint(embeddedTexture->achFormatHint);
                            formatExt = L"." + std::wstring(hint.begin(), hint.end());
                        }

                        std::shared_ptr<Texture> matTex = nullptr;

                        std::string embeddedTexPathStr = "[Texture]-" + path_str;

                        if (ResourceManager::Get()->Containskey(embeddedTexPathStr))
                        {
                            matTex = ResourceManager::Get()->Load<Texture>(embeddedTexPathStr);
                            resourceMat->InitTexture(myType, slot, matTex);
                        }
                        else
                        {
                            matTex = ResourceManager::Get()->Load<Texture>(embeddedTexPathStr);
                            matTex->LoadTextureFromMemory(s_pContext, embeddedTexPathStr, data, size, formatExt, use_sRGB);
                            resourceMat->InitTexture(myType, slot, matTex);
                        }
                    }
                }
                else
                {
                    fs::path original_path(path.C_Str());
                    fs::path filename_only = original_path.filename();
                    fs::path final_texPath = base_directory / filename_only;

                    std::string filenameStr = filename_only.string();

                    std::shared_ptr<Texture> matTex = nullptr;

                    if (ResourceManager::Get()->Containskey(filenameStr))
                    {
                        matTex = ResourceManager::Get()->Load<Texture>(filenameStr);
                        resourceMat->InitTexture(myType, slot, matTex);
                    }
                    else
                    {
                        matTex = ResourceManager::Get()->Load<Texture>(filenameStr);

                        matTex->LoadTextureFromFile(s_pContext, filenameStr, final_texPath.wstring(), use_sRGB);
                        resourceMat->InitTexture(myType, slot, matTex);
                    }
                }
            }
        }
    }

    // 프로퍼티 불러오기 (전역 설정에 의해 결정)
    aiColor4D propertyColor;
    ai_real propertyKey;
    switch (s_materialProperties)
    {
    case LoadMaterialProperties::All:
        if (AI_SUCCESS == pMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, propertyKey)
            && (resourceMat->GetTextureFlags() & static_cast<UINT>(TextureType::Roughness)) == 0)
        {
            resourceMat->SetRoughnessKey({ propertyKey });
        }
        if (AI_SUCCESS == pMat->Get(AI_MATKEY_REFLECTIVITY, propertyKey)
            && (resourceMat->GetTextureFlags() & static_cast<UINT>(TextureType::Metalness)) == 0)
        {
            resourceMat->SetMetallicKey({ propertyKey });
        }
        [[fallthrough]];
    case LoadMaterialProperties::OnlyBaseColor:
        if (AI_SUCCESS == pMat->Get(AI_MATKEY_COLOR_DIFFUSE, propertyColor))
        {
            resourceMat->SetBaseColor({ propertyColor.r, propertyColor.g, propertyColor.b, propertyColor.a });
   //         if(s_materialType == LoadMaterialType::BRDF)
   //         {
   //             // BRDF 머티리얼의 경우 알베도 색상도 설정
   //             resourceMat->SetBaseColor({ std::pow(propertyColor.r,2.2f), std::pow(propertyColor.g,2.2f), std::pow(propertyColor.b,2.2f), propertyColor.a });
			//}
        }
        [[fallthrough]];
    case LoadMaterialProperties::None:
        break;
    }

    resourceMat->CreateConstantBuffer(s_pContext);
}

void MyEngine::AssimpConverter::Initialize(ID3D11DeviceContext* context)
{
    s_pContext = context;
    s_pContext->GetDevice(&s_pDevice);
}

void MyEngine::AssimpConverter::Release()
{
    if (s_pDevice)
    {
        s_pDevice->Release();
        s_pDevice = nullptr;
    }
}

std::unique_ptr<MyEngine::StaticMeshRenderer> MyEngine::AssimpConverter::LoadStaticMeshRendererFromFile(std::string filePath)
{
    //importFlag 세팅
    s_importFlags = 
        aiProcess_GlobalScale |
        aiProcess_Triangulate |    // vertex 삼각형 으로 출력
        aiProcess_GenNormals |        // Normal 정보 생성  
        aiProcess_GenUVCoords |      // 텍스처 좌표 생성
        aiProcess_CalcTangentSpace |  // 탄젠트 벡터 생성
        aiProcess_JoinIdenticalVertices |  // 중복 정점 제거
        aiProcess_ValidateDataStructure | // 구조 검증
        //aiProcess_ConvertToLeftHanded |  // DX용 왼손좌표계 변환 <- 제외사유 : SimpleMath로 구현한 트랜스폼 클래스 때문에 이미 오른손좌표계임
        aiProcess_PreTransformVertices |  // 노드의 변환행렬을 적용한 버텍스 생성한다.  *StaticMesh로 처리할때만
        aiProcess_FlipUVs;

    std::string fileName = "[Model]-" + std::string{ filePath.c_str() };

    std::shared_ptr<AssimpModel> model = nullptr;
    if (ResourceManager::Get()->Containskey(fileName))
    {
        model = ResourceManager::Get()->Load<AssimpModel>(fileName);
    }
    else
    {
        model = ResourceManager::Get()->Load<AssimpModel>(fileName);
        model->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 5.0f);
        model->importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
        model->LoadFromFile(filePath.c_str(), s_importFlags);
    }


    if (!model->pScene) {
        throw std::runtime_error("model load error! :: check model file - " + std::string(model->importer.GetErrorString()));
    }
    auto sceneName = std::string{ model->pScene->mName.C_Str() };

    auto pStaticMeshRenderer = std::make_unique<StaticMeshRenderer>();
    std::shared_ptr<StaticMesh> sMesh = nullptr;

    std::string meshName = "[StaticMesh]-" + sceneName + filePath;

    // 리소스 테이블에 찾는 메쉬가 있는지 체크
    if (ResourceManager::Get()->Containskey(meshName))
    {
        sMesh = ResourceManager::Get()->Load<StaticMesh>(meshName);
        pStaticMeshRenderer->SetMesh(sMesh);
    }
    else
    {
        sMesh = ResourceManager::Get()->Load<StaticMesh>(meshName);
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<UINT> matIndices;

        // 없으면 리소스 테이블에 등록한 직 후 노드 돌기 
        ProcessNode(meshes, matIndices, model->pScene->mRootNode, model->pScene);

        for (auto& mesh : meshes)
        {
            mesh->CreateBuffers(s_pDevice);
        }

        sMesh->SetSubMesh(std::move(meshes));
        sMesh->CalcBBox();
        sMesh->SetMatRefIndices(std::move(matIndices));
        pStaticMeshRenderer->SetMesh(sMesh);
    }

    std::string shaderType = "";
    switch (s_materialType)
    {
    case LoadMaterialType::BlinnPhong:
        shaderType = "[BlinnPhong] |";
        break;
    case LoadMaterialType::BlinnPhongToon:
        shaderType = "[BlinnPhongToon] |";
        break;
    case LoadMaterialType::BRDF:
        shaderType = "[BRDF] |";
        break;
    }

    for (UINT i = 0; i < model->pScene->mNumMaterials; i++)
    {
        std::string matName = "[Static] | " + shaderType + std::string{model->pScene->mMaterials[i]->GetName().C_Str()};

        std::shared_ptr<Material> mat = nullptr;

        if (ResourceManager::Get()->Containskey(matName))
        {
            mat = ResourceManager::Get()->Load<Material>(matName);
        }
        else
        {
            mat = ResourceManager::Get()->Load<Material>(matName);
            ProcessMaterial(model->pScene->mMaterials[i], mat, model->pScene, BoneType::None);
        }

        pStaticMeshRenderer->AddMaterial(mat);
    }

    return pStaticMeshRenderer;
}


std::unique_ptr<MyEngine::RigidMeshRenderer> MyEngine::AssimpConverter::LoadRigidMeshRendererFromFile(std::string filePath)
{
    //importFlag 세팅
    s_importFlags =
        aiProcess_GlobalScale |
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_FlipUVs;

    std::string fileName = "[Model]-" + std::string{ filePath.c_str() };

    std::shared_ptr<AssimpModel> model = nullptr;
    if (ResourceManager::Get()->Containskey(fileName))
    {
        model = ResourceManager::Get()->Load<AssimpModel>(fileName);
    }
    else
    {
        model = ResourceManager::Get()->Load<AssimpModel>(fileName);
        model->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 5.0f);
        model->LoadFromFile(filePath.c_str(), s_importFlags);
    }


    if (!model->pScene) {
        throw std::runtime_error("model load error! :: check model file - " + std::string(model->importer.GetErrorString()));
    }

    auto sceneName = std::string{ model->pScene->mName.C_Str() };
    auto pRigidMeshRenderer = std::make_unique<RigidMeshRenderer>();

    std::shared_ptr<RigidMesh> rMesh = nullptr;

    std::string meshName = "[RigidMesh]-" + sceneName + filePath;

    // 리소스 테이블에 찾는 메쉬가 있는지 체크
    if (ResourceManager::Get()->Containskey(meshName))
    {
        rMesh = ResourceManager::Get()->Load<RigidMesh>(meshName);
        pRigidMeshRenderer->SetMesh(rMesh);
        std::vector<RigidBonePose>& initRigidBonePoses = rMesh->GetInitBonePoses();

        std::vector<RigidBonePose> rigidBonePoses;
        rigidBonePoses.resize(initRigidBonePoses.size());
        std::memcpy(rigidBonePoses.data(), initRigidBonePoses.data(), sizeof(RigidBonePose) * initRigidBonePoses.size());

        pRigidMeshRenderer->SetBonePoses(std::move(rigidBonePoses));
        pRigidMeshRenderer->CreateBoneMatrixBuffer(s_pContext);
    }
    else
    {
        rMesh = ResourceManager::Get()->Load<RigidMesh>(meshName);
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<UINT> matIndices;
        std::vector<UINT> boneIndices;
        std::vector<RigidBone> rigidBones;
        std::vector<RigidBonePose> initRigidBonePoses;


        std::unordered_map <std::string, UINT> nodeNameToIndex;

        ProcessNode(-1, rigidBones, initRigidBonePoses, meshes, matIndices, boneIndices, model->pScene->mRootNode, model->pScene, nodeNameToIndex);

        for (auto& mesh : meshes)
        {
            mesh->CreateBuffers(s_pDevice);
        }

        std::vector<RigidBonePose> rigidBonePoses;
        rigidBonePoses.resize(initRigidBonePoses.size());
        std::memcpy(rigidBonePoses.data(), initRigidBonePoses.data(), sizeof(RigidBonePose) * initRigidBonePoses.size());

        rMesh->SetSubMesh(std::move(meshes));
        rMesh->SetBones(std::move(rigidBones));
        rMesh->SetBoneIndices(std::move(boneIndices));
        rMesh->SetMatRefIndices(std::move(matIndices));
        rMesh->SetBoneNameToIdxMap(std::move(nodeNameToIndex));

        pRigidMeshRenderer->SetBonePoses(std::move(rigidBonePoses));
        pRigidMeshRenderer->SetMesh(rMesh);
        pRigidMeshRenderer->CreateBoneMatrixBuffer(s_pContext);
    }

    if (model->pScene->HasAnimations())
    {
        std::vector<std::shared_ptr<AnimationClip>> boneAnimations;
        boneAnimations.resize(model->pScene->mNumAnimations);

        auto& nodeNameToIndex = rMesh->GetBoneNameToIdxMap();

        for (UINT i = 0; i < model->pScene->mNumAnimations; i++)
        {
            auto& animation = model->pScene->mAnimations[i];

            std::string animationClipName = "[AnimationClip]-" + sceneName + std::string{ animation->mName.C_Str() };
            std::shared_ptr<AnimationClip> animationClip = nullptr;
            if (ResourceManager::Get()->Containskey(animationClipName))
            {
                animationClip = ResourceManager::Get()->Load<AnimationClip>(animationClipName);
                boneAnimations[i] = animationClip;
            }
            else
            {
                animationClip = ResourceManager::Get()->Load<AnimationClip>(animationClipName);
                for (UINT j = 0; j < animation->mNumChannels; j++)
                {
                    auto& animNode = animation->mChannels[j];

                    //std::cout << animation->mName.C_Str() << std::endl;

                    //std::cout << "    ->" << animNode->mNodeName.C_Str() << std::endl;

                    //if (nodeNameToIndex.find({ animNode->mNodeName.C_Str() }) != nodeNameToIndex.end())
                    //    std::cout << "    nameMatching!" << std::endl;

                    auto currentNodeIdx = nodeNameToIndex[{ animNode->mNodeName.C_Str() }];
                    auto& currentAnimationClip = animationClip->channels[currentNodeIdx] = {};
                    currentAnimationClip.frameRate = animation->mTicksPerSecond;
                    if (animation->mTicksPerSecond != 0.0)
                        currentAnimationClip.duration = static_cast<float>(animation->mDuration / animation->mTicksPerSecond);
                    else
                        currentAnimationClip.duration = static_cast<float>(animation->mDuration);

                    //std::cout << "        # posKeys : " << animNode->mNumPositionKeys << std::endl;
                    for (UINT k = 0; k < animNode->mNumPositionKeys; k++)
                    {
                        const auto& posKey = animNode->mPositionKeys[k];
                        currentAnimationClip.pos.AddKeyframe(
                            posKey.mTime,
                            Vector3
                            {
                                static_cast<float>(posKey.mValue.x),
                                static_cast<float>(posKey.mValue.y),
                                static_cast<float>(posKey.mValue.z)
                            }
                        );

                        //std::cout << "        [pos] - { "
                        //    << posKey.mValue.x
                        //    << ", "
                        //    << posKey.mValue.y
                        //    << ", "
                        //    << posKey.mValue.z
                        //    << "} " << std::endl;
                    }
                    //std::cout << "    ==============================" << std::endl;
                    //std::cout << "        # rotKeys : " << animNode->mNumRotationKeys << std::endl;
                    for (UINT k = 0; k < animNode->mNumRotationKeys; k++)
                    {
                        const auto& rotKey = animNode->mRotationKeys[k];
                        currentAnimationClip.rot.AddKeyframe(
                            rotKey.mTime,
                            Quaternion
                            {
                                static_cast<float>(rotKey.mValue.x),
                                static_cast<float>(rotKey.mValue.y),
                                static_cast<float>(rotKey.mValue.z),
                                static_cast<float>(rotKey.mValue.w)
                            }
                        );

                        //std::cout << "        [rot] - { "
                        //    << animNode->mRotationKeys[k].mValue.x
                        //    << ", "
                        //    << animNode->mRotationKeys[j].mValue.y
                        //    << ", "
                        //    << animNode->mRotationKeys[j].mValue.z
                        //    << ", "
                        //    << animNode->mRotationKeys[j].mValue.w
                        //    << "} " << std::endl;
                    }
                    //std::cout << "    ==============================" << std::endl;
                    //std::cout << "        # scaleKeys : " << animNode->mNumScalingKeys << std::endl;
                    for (UINT k = 0; k < animNode->mNumScalingKeys; k++)
                    {
                        const auto& scaleKey = animNode->mScalingKeys[k];
                        currentAnimationClip.scale.AddKeyframe(
                            scaleKey.mTime,
                            Vector3
                            {
                                static_cast<float>(scaleKey.mValue.x),
                                static_cast<float>(scaleKey.mValue.y),
                                static_cast<float>(scaleKey.mValue.z)
                            }
                        );

                        //std::cout << "        [scale] - { "
                        //    << animNode->mScalingKeys[k].mValue.x
                        //    << ", "
                        //    << animNode->mScalingKeys[k].mValue.y
                        //    << ", "
                        //    << animNode->mScalingKeys[k].mValue.z
                        //    << "} " << std::endl;
                    }
                }
            }
            boneAnimations[i] = animationClip;
        }

        pRigidMeshRenderer->SetAnimations(std::move(boneAnimations));
    }

    std::string shaderType = "";
    switch (s_materialType)
    {
    case LoadMaterialType::BlinnPhong:
        shaderType = "[BlinnPhong] |";
        break;
    case LoadMaterialType::BlinnPhongToon:
        shaderType = "[BlinnPhongToon] |";
        break;
    case LoadMaterialType::BRDF:
        shaderType = "[BRDF] |";
        break;
    }

    for (UINT i = 0; i < model->pScene->mNumMaterials; i++)
    {


        std::string matName = "[RigidBone] | " + shaderType + std::string{model->pScene->mMaterials[i]->GetName().C_Str()};

        std::shared_ptr<Material> mat = nullptr;

        if (ResourceManager::Get()->Containskey(matName))
        {
            mat = ResourceManager::Get()->Load<Material>(matName);
        }
        else
        {
            mat = ResourceManager::Get()->Load<Material>(matName);
            ProcessMaterial(model->pScene->mMaterials[i], mat, model->pScene, BoneType::RigidBone);
        }

        pRigidMeshRenderer->AddMaterial(mat);
    }

    return pRigidMeshRenderer;
}

std::unique_ptr<MyEngine::SkinningMeshRenderer> MyEngine::AssimpConverter::LoadSkinningMeshRendererFromFile(std::string filePath)
{
    // importFlag 세팅
    s_importFlags =
        aiProcess_GlobalScale |
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs;

    std::string fileName = "[Model]-" + std::string{ filePath.c_str() };


    std::shared_ptr<AssimpModel> model = nullptr;
    if (ResourceManager::Get()->Containskey(fileName))
    {
        model = ResourceManager::Get()->Load<AssimpModel>(fileName);
    }
    else
    {
        model = ResourceManager::Get()->Load<AssimpModel>(fileName);
        model->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 5.0f);
        model->importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
        model->LoadFromFile(filePath.c_str(), s_importFlags);

        s_loadedModels.push_back(model);
    }


    if (!model->pScene) {
        throw std::runtime_error("model load error! :: check model file - " + std::string(model->importer.GetErrorString()));
    }

    auto sceneName = std::string{ model->pScene->mName.C_Str() };



    auto pSkinningMeshRenderer = std::make_unique<SkinningMeshRenderer>();
    std::shared_ptr<SkinningMesh> sMesh = nullptr;
    std::string meshName = "[SkinningMesh]-" + sceneName + filePath;

    if (ResourceManager::Get()->Containskey(meshName))
    {
        sMesh = ResourceManager::Get()->Load<SkinningMesh>(meshName);
        pSkinningMeshRenderer->SetMesh(sMesh);

        std::vector<SkinningBonePose>& initSkinningBonePoses = sMesh->GetInitBonePoses();


        std::vector<SkinningBonePose> skinningBonePoses;
        skinningBonePoses.resize(initSkinningBonePoses.size());
        std::memcpy(skinningBonePoses.data(), initSkinningBonePoses.data(), sizeof(RigidBonePose) * initSkinningBonePoses.size());

        pSkinningMeshRenderer->SetBonePoses(std::move(skinningBonePoses));
        pSkinningMeshRenderer->MatrixUpdate();
        pSkinningMeshRenderer->CreateBoneModelMatrixBuffer(s_pContext);
    }
    else
    {
        sMesh = ResourceManager::Get()->Load<SkinningMesh>(meshName);

        auto usedBones = CollectUsedBoneNames(model->pScene);
        std::unordered_set<std::string> boneHierarchy;
        CollectBoneHierarchy(model->pScene->mRootNode, usedBones, boneHierarchy);

        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<UINT> matIndices;
        std::vector<SkinningBone> skinningBones;
        std::vector<SkinningBonePose> initSkinningBonePoses;
        std::vector<CorrectionNode> correctionMap;

        std::unordered_map<std::string, UINT> nodeNameToIndex;
        ProcessNode(-1, skinningBones, initSkinningBonePoses, meshes, matIndices, model->pScene->mRootNode, model->pScene, nodeNameToIndex, correctionMap, boneHierarchy);

        struct VertexBoneData
        {
            std::vector<std::pair<UINT, float>> boneWeights; // {boneIndex, weight}
            void AddBoneData(UINT boneIndex, float weight)
            {
                //이후 4개만 남기기
                boneWeights.emplace_back(boneIndex, weight);
            }
        };

        // [메쉬 인덱스] -> [정점 인덱스] -> [뼈대 데이터 리스트]
        std::vector<std::vector<VertexBoneData>> allMeshBoneData;
        allMeshBoneData.resize(meshes.size());

        // 각 메쉬의 정점 수에 맞춰 내부 벡터 초기화
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            allMeshBoneData[i].resize(meshes[i]->GetVertices().size());
        }

        for (size_t i = 0; i < correctionMap.size(); i++)
        {
            auto& node = correctionMap[i];
            auto boneName = std::string{ node.pBone->mName.C_Str() };
            // 이전에 ProcessNode에서 올바르게 설정된 메쉬 인덱스를 사용
            UINT meshIdx = node.meshIdx;

            if (nodeNameToIndex.find(boneName) == nodeNameToIndex.end()) continue;

            UINT boneIdx = nodeNameToIndex[boneName];

            auto& matrix = node.pBone->mOffsetMatrix;
            skinningBones[boneIdx].offset = Matrix(
                matrix.a1, matrix.a2, matrix.a3, matrix.a4,
                matrix.b1, matrix.b2, matrix.b3, matrix.b4,
                matrix.c1, matrix.c2, matrix.c3, matrix.c4,
                matrix.d1, matrix.d2, matrix.d3, matrix.d4
            );


            // 정점 가중치 정보 누적
            for (UINT j = 0; j < node.pBone->mNumWeights; j++)
            {
                const auto& weight = node.pBone->mWeights[j];
                allMeshBoneData[meshIdx][weight.mVertexId].AddBoneData(boneIdx, weight.mWeight);
            }
        }

        for (UINT meshIdx = 0; meshIdx < meshes.size(); ++meshIdx)
        {
            auto& currentMeshVertices = meshes[meshIdx]->GetVertices();
            auto& meshBoneData = allMeshBoneData[meshIdx];

            for (UINT vertIdx = 0; vertIdx < currentMeshVertices.size(); ++vertIdx)
            {
                auto& vertex = currentMeshVertices[vertIdx];
                auto& boneData = meshBoneData[vertIdx].boneWeights;

                // 가중치 순으로 내림차순 정렬하여 가장 큰 4개의 뼈대만 선택
                std::sort(boneData.begin(), boneData.end(), [](const auto& a, const auto& b) {
                    return a.second > b.second;
                    });

                float totalWeight = 0.0f;

                // 최대 4개의 뼈대 인덱스와 가중치를 정점 구조체에 할당
                for (int i = 0; i < 4; ++i)
                {
                    if (i < boneData.size())
                    {
                        vertex.boneIndices[i] = boneData[i].first;
                        vertex.boneWeights[i] = boneData[i].second;
                        totalWeight += boneData[i].second;

                    }
                    else
                    {
                        // 4개 미만인 경우 나머지 초기화
                        vertex.boneIndices[i] = 0;
                        vertex.boneWeights[i] = 0.0f;
                    }
                }

                // 가중치 정규화
                if (totalWeight > 0.0f)
                {
                    float factor = 1.0f / totalWeight;
                    for (int i = 0; i < 4; ++i)
                    {
                        vertex.boneWeights[i] *= factor;
                        auto& currentBone = skinningBones[vertex.boneIndices[i]];

                        if (!currentBone.hasVertex)
                        {
                            currentBone.hasVertex = true;

                            XMVECTOR pos = XMLoadFloat3(&vertex.position);
                            BoundingBox::CreateFromPoints(currentBone.bbox, pos, pos);
                        }
                        else
                        {
                            // 기존 박스와 병합
                            BoundingBox temp;
                            XMVECTOR pos = XMLoadFloat3(&vertex.position);
                            BoundingBox::CreateFromPoints(temp, pos, pos);
                            BoundingBox::CreateMerged(currentBone.bbox, currentBone.bbox, temp);
                        }
                    }
                }
            }
        }

        for (auto& mesh : meshes)
        {
            mesh->CreateBuffers(s_pDevice);
        }

        for (auto& sbone : skinningBones)
        {
            if (!sbone.hasVertex)
                continue;

            auto& bbox = sbone.bbox;
            bbox.Transform(bbox, sbone.offset.Transpose());
        }

        std::vector<SkinningBonePose> skinningBonePoses;
        skinningBonePoses.resize(initSkinningBonePoses.size());
        std::memcpy(skinningBonePoses.data(), initSkinningBonePoses.data(), sizeof(RigidBonePose) * initSkinningBonePoses.size());


        sMesh->SetSubMesh(std::move(meshes));
        sMesh->SetBones(std::move(skinningBones));
        sMesh->CreateBoneOffsetMatrixBuffer(s_pContext);
        sMesh->CalcBBox();
        sMesh->SetMatRefIndices(std::move(matIndices));
        sMesh->SetInitBonePoses(std::move(initSkinningBonePoses));
        sMesh->SetBoneNameToIdxMap(std::move(nodeNameToIndex));

        pSkinningMeshRenderer->SetMesh(sMesh);
        pSkinningMeshRenderer->SetBonePoses(std::move(skinningBonePoses));
        pSkinningMeshRenderer->MatrixUpdate();
        pSkinningMeshRenderer->CreateBoneModelMatrixBuffer(s_pContext);
    }

   

    if (model->pScene->HasAnimations())
    {
        std::vector<std::shared_ptr<AnimationClip>> boneAnimations;
        boneAnimations.resize(model->pScene->mNumAnimations);

        auto& nodeNameToIndex = sMesh->GetBoneNameToIdxMap();

        for (UINT i = 0; i < model->pScene->mNumAnimations; i++)
        {
            auto& animation = model->pScene->mAnimations[i];

            std::string animationClipName = "[AnimationClip]-" + sceneName + std::string{ animation->mName.C_Str() };
            std::shared_ptr<AnimationClip> animationClip = nullptr;
            if (ResourceManager::Get()->Containskey(animationClipName))
            {
                animationClip = ResourceManager::Get()->Load<AnimationClip>(animationClipName);
                boneAnimations[i] = animationClip;
            }
            else
            {
                animationClip = ResourceManager::Get()->Load<AnimationClip>(animationClipName);
                for (UINT j = 0; j < animation->mNumChannels; j++)
                {
                    auto& animNode = animation->mChannels[j];

                    //std::cout << animation->mName.C_Str() << std::endl;

                    //std::cout << "    ->" << animNode->mNodeName.C_Str() << std::endl;

                    //if (nodeNameToIndex.find({ animNode->mNodeName.C_Str() }) != nodeNameToIndex.end())
                    //    std::cout << "    nameMatching!" << std::endl;

                    auto currentNodeIdx = nodeNameToIndex[{ animNode->mNodeName.C_Str() }];
                    auto& currentAnimationClip = animationClip->channels[currentNodeIdx] = {};
                    currentAnimationClip.frameRate = animation->mTicksPerSecond;
                    if (animation->mTicksPerSecond != 0.0)
                        currentAnimationClip.duration = static_cast<float>(animation->mDuration / animation->mTicksPerSecond);
                    else
                        currentAnimationClip.duration = static_cast<float>(animation->mDuration);

                    //std::cout << "        # posKeys : " << animNode->mNumPositionKeys << std::endl;
                    for (UINT k = 0; k < animNode->mNumPositionKeys; k++)
                    {
                        const auto& posKey = animNode->mPositionKeys[k];
                        currentAnimationClip.pos.AddKeyframe(
                            posKey.mTime,
                            Vector3
                            {
                                static_cast<float>(posKey.mValue.x),
                                static_cast<float>(posKey.mValue.y),
                                static_cast<float>(posKey.mValue.z)
                            }
                        );

                        //std::cout << "        [pos] - { "
                        //    << posKey.mValue.x
                        //    << ", "
                        //    << posKey.mValue.y
                        //    << ", "
                        //    << posKey.mValue.z
                        //    << "} " << std::endl;
                    }
                    //std::cout << "    ==============================" << std::endl;
                    //std::cout << "        # rotKeys : " << animNode->mNumRotationKeys << std::endl;
                    for (UINT k = 0; k < animNode->mNumRotationKeys; k++)
                    {
                        const auto& rotKey = animNode->mRotationKeys[k];
                        currentAnimationClip.rot.AddKeyframe(
                            rotKey.mTime,
                            Quaternion
                            {
                                static_cast<float>(rotKey.mValue.x),
                                static_cast<float>(rotKey.mValue.y),
                                static_cast<float>(rotKey.mValue.z),
                                static_cast<float>(rotKey.mValue.w)
                            }
                        );

                        //std::cout << "        [rot] - { "
                        //    << animNode->mRotationKeys[k].mValue.x
                        //    << ", "
                        //    << animNode->mRotationKeys[j].mValue.y
                        //    << ", "
                        //    << animNode->mRotationKeys[j].mValue.z
                        //    << ", "
                        //    << animNode->mRotationKeys[j].mValue.w
                        //    << "} " << std::endl;
                    }
                    //std::cout << "    ==============================" << std::endl;
                    //std::cout << "        # scaleKeys : " << animNode->mNumScalingKeys << std::endl;
                    for (UINT k = 0; k < animNode->mNumScalingKeys; k++)
                    {
                        const auto& scaleKey = animNode->mScalingKeys[k];
                        currentAnimationClip.scale.AddKeyframe(
                            scaleKey.mTime,
                            Vector3
                            {
                                static_cast<float>(scaleKey.mValue.x),
                                static_cast<float>(scaleKey.mValue.y),
                                static_cast<float>(scaleKey.mValue.z)
                            }
                        );

                        //std::cout << "        [scale] - { "
                        //    << animNode->mScalingKeys[k].mValue.x
                        //    << ", "
                        //    << animNode->mScalingKeys[k].mValue.y
                        //    << ", "
                        //    << animNode->mScalingKeys[k].mValue.z
                        //    << "} " << std::endl;
                    }
                }
            }
            boneAnimations[i] = animationClip;
        }

        pSkinningMeshRenderer->SetAnimations(std::move(boneAnimations));
    }

	std::string shaderType = "";
    switch (s_materialType)
    {
	case LoadMaterialType::BlinnPhong:
		shaderType = "[BlinnPhong] |";
		break;
	case LoadMaterialType::BlinnPhongToon:
		shaderType = "[BlinnPhongToon] |";
		break;
	case LoadMaterialType::BRDF:
		shaderType = "[BRDF] |";
		break;
    }

    for (UINT i = 0; i < model->pScene->mNumMaterials; i++)
    {

        std::string matName = "[SkinnningBone] | " + shaderType + std::string{model->pScene->mMaterials[i]->GetName().C_Str()};

        std::shared_ptr<Material> mat = nullptr;

        if (ResourceManager::Get()->Containskey(matName))
        {
            mat = ResourceManager::Get()->Load<Material>(matName);
        }
        else
        {
            mat = ResourceManager::Get()->Load<Material>(matName);
            ProcessMaterial(model->pScene->mMaterials[i], mat, model->pScene, BoneType::SkinningBone);
        }
        pSkinningMeshRenderer->AddMaterial(mat);
    }

    return pSkinningMeshRenderer;
}

MyEngine::AssimpConverter::LoadMaterialType MyEngine::AssimpConverter::s_materialType = MyEngine::AssimpConverter::LoadMaterialType::BlinnPhong;
MyEngine::AssimpConverter::LoadMaterialProperties MyEngine::AssimpConverter::s_materialProperties = MyEngine::AssimpConverter::LoadMaterialProperties::None;

void MyEngine::AssimpConverter::SetLoadMaterialType(LoadMaterialType type)
{
    s_materialType = type;
}

void MyEngine::AssimpConverter::SetLoadMaterialProperties(LoadMaterialProperties props)
{
    s_materialProperties = props;
}
