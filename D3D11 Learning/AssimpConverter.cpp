#include "AssimpConverter.h"
#include <queue>
#include <stdexcept>
#include <filesystem>

std::unique_ptr<Assimp::Importer> MyEngine::AssimpConverter::s_importer = nullptr;
uint32_t MyEngine::AssimpConverter::s_importFlags = 0;
ID3D11Device* MyEngine::AssimpConverter::s_pDevice = nullptr;
ID3D11DeviceContext* MyEngine::AssimpConverter::s_pContext = nullptr;

namespace fs = std::filesystem;

//#include <iostream>

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

void MyEngine::AssimpConverter::ProcessNode(std::vector<Mesh>& meshes,std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene)
{
    //메쉬 정보 처리
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        meshes.push_back(ProcessMesh(pMesh, pScene));
        matIDX.push_back(pMesh->mMaterialIndex);
    }

    //자식 노드 처리
    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        ProcessNode(meshes, matIDX, pNode->mChildren[i], pScene);
    }
}

void MyEngine::AssimpConverter::ProcessNode(int parentIndex, std::vector<RigidBone>& bones, std::vector<Mesh>& meshes, std::vector<UINT>& matIDX, std::vector<UINT>& boneIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string, UINT>& nodeNameToIndexMap)
{
    //메쉬 정보 처리
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        meshes.push_back(ProcessMesh(pMesh, pScene));
        matIDX.push_back(pMesh->mMaterialIndex);
        boneIDX.push_back(static_cast<int>(bones.size()));
        //std::cout << boneIDX.size() << " : " << pMesh->mName.C_Str() << "[ parent : " << (currentDepth - 1) << " ]" << std::endl;
    }

    RigidBone bone;

    auto& matrix = pNode->mTransformation;
    bone.index = static_cast<int>(bones.size());
    bone.parentIndex = parentIndex;
    bone.local = Matrix(
        matrix.a1, matrix.a2, matrix.a3, matrix.a4,
        matrix.b1, matrix.b2, matrix.b3, matrix.b4,
        matrix.c1, matrix.c2, matrix.c3, matrix.c4,
        matrix.d1, matrix.d2, matrix.d3, matrix.d4
    );

    bones.emplace_back(bone);
    nodeNameToIndexMap.insert({ pNode->mName.C_Str(),bone.index });

    //자식 노드 처리
    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        ProcessNode(bone.index, bones, meshes, matIDX, boneIDX,pNode->mChildren[i], pScene, nodeNameToIndexMap);
    }
}

void MyEngine::AssimpConverter::ProcessNode(int parentIndex, std::vector<SkinningBone>& bones, std::vector<Mesh>& meshes, std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene, std::unordered_map<std::string, UINT>& nodeNameToIndexMap, std::vector<CorrectionNode>& correctionMap, const std::unordered_set<std::string>& boneHierarchy)
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
        bone.local = Matrix(
            matrix.a1, matrix.a2, matrix.a3, matrix.a4,
            matrix.b1, matrix.b2, matrix.b3, matrix.b4,
            matrix.c1, matrix.c2, matrix.c3, matrix.c4,
            matrix.d1, matrix.d2, matrix.d3, matrix.d4
        );

        bone.boundBox = { {FLT_MAX,FLT_MAX,FLT_MAX},{-FLT_MAX,-FLT_MAX,-FLT_MAX} };

        bones.emplace_back(bone);
        nodeNameToIndexMap.insert({ nodeName, bone.index });
        currentBoneIndex = bone.index;
    }

    //메쉬 정보 처리
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        meshes.push_back(ProcessMesh(pMesh, pScene));
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
        ProcessNode(childParentIndex, bones, meshes, matIDX,
            pNode->mChildren[i], pScene, nodeNameToIndexMap,
            correctionMap, boneHierarchy);
    }
}


MyEngine::Mesh MyEngine::AssimpConverter::ProcessMesh(aiMesh* pMesh, const aiScene* pScene)
{
    std::vector<VertexType> vertices;
    std::vector<UINT> indices;
    AABB aabb;

	float minx, miny, minz;
	float maxx, maxy, maxz;
	minx = miny = minz = FLT_MAX;
	maxx = maxy = maxz = -FLT_MAX;

    for (UINT i = 0; i < pMesh->mNumVertices; i++)
    {
        VertexType vertex;

        vertex.position = { pMesh->mVertices[i].x,pMesh->mVertices[i].y,pMesh->mVertices[i].z };
        
		if (vertex.position.x < minx) minx = vertex.position.x;
		if (vertex.position.y < miny) miny = vertex.position.y;
		if (vertex.position.z < minz) minz = vertex.position.z;
		if (vertex.position.x > maxx) maxx = vertex.position.x;
		if (vertex.position.y > maxy) maxy = vertex.position.y;
		if (vertex.position.z > maxz) maxz = vertex.position.z;

        vertex.normal = { pMesh->mNormals[i].x,pMesh->mNormals[i].y,pMesh->mNormals[i].z};
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

    aabb.min = { minx,miny,minz };
    aabb.max = { maxx,maxy,maxz };

    return Mesh(vertices, indices, aabb);
}

MyEngine::Material MyEngine::AssimpConverter::ProcessMaterial(aiMaterial* pMat, const aiScene* pScene, const BoneType& boneType)
{
    Material mat{ { pMat->GetName().C_Str()} };

    mat.InitSampler(s_pDevice);

    switch (boneType)
    {
    case BoneType::None:
        mat.InitShader(ShaderType::Vertex, Material::GetBlinnPhongVertexShader(), Material::GetBlinnPhongVSBlob());
        break;
    case BoneType::RigidBone:
        mat.InitShader(ShaderType::Vertex, Material::GetBlinnPhongVertexShader_RigidBone(), Material::GetBlinnPhongVSBlob());
        break;
    case BoneType::SkinningBone:
        mat.InitShader(ShaderType::Vertex, Material::GetBlinnPhongVertexShader_SkinningBone(), Material::GetBlinnPhongVSBlob());
        break;
    }

    switch (s_materialType)
    {
    case LoadMaterialType::BlinnPhong:
        mat.InitShader(ShaderType::Pixel, Material::GetBlinnPhongPixelShader(), nullptr);
        break;
    case LoadMaterialType::BlinnPhongToon:
        mat.InitShader(ShaderType::Pixel, Material::GetBlinnPhongToonPixelShader(), nullptr);
        break;
    }

    //색상 불러오기
    aiColor4D diffuseColor;
    if (AI_SUCCESS == pMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor))
    {
        mat.SetBaseColor({ diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a });
    }
    else
    {
        mat.SetBaseColor({ 1.0f, 1.0f, 1.0f, 1.0f });
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
        //{ aiTextureType_METALNESS, TextureType::Metalness, 6 },
        //{ aiTextureType_DIFFUSE_ROUGHNESS, TextureType::Roughness, 7 },
    };

    static fs::path base_directory = std::filesystem::current_path() / "Resources/Textures";

    for (const auto& [aiType, myType, slot] : textureTypes)
    {
        const UINT count = pMat->GetTextureCount(aiType);
        for (UINT i = 0; i < count; ++i)
        {
            aiString path;
            if (pMat->GetTexture(aiType, i, &path) == AI_SUCCESS)
            {
                std::string path_str = path.C_Str();

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

                        mat.InitAndConvertTextureFromMemory(
                            s_pContext,
                            myType,
                            path_str,
                            slot,
                            data,       // 일반화된 포인터
                            size,       // 일반화된 크기
                            formatExt   // 일반화된 확장자
                        );
                    }
                }
                else
                {
                    fs::path original_path(path.C_Str());
                    fs::path filename_only = original_path.filename();
                    fs::path final_texPath = base_directory / filename_only;

                    mat.InitAndConvertTexture(
                        s_pContext,
                        myType,
                        filename_only.string(),
                        slot,
                        final_texPath.wstring()
                    );
                }
            }
        }
    }

    return mat;
}

void MyEngine::AssimpConverter::Initialize(ID3D11DeviceContext* context)
{
    s_pContext = context;
    s_pContext->GetDevice(&s_pDevice);
    s_importer = std::make_unique<Assimp::Importer>();
}

void MyEngine::AssimpConverter::Release()
{
    s_importer.reset();
}

std::unique_ptr<MyEngine::RigidMeshRenderer> MyEngine::AssimpConverter::LoadRigidMeshRendererFromFile(std::string filePath)
{
    //importFlag 세팅
    s_importFlags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_FlipUVs;
    

    Material::InitBlinnPhongShaders(s_pDevice);

    const aiScene* pScene = s_importer->ReadFile(filePath.c_str(), s_importFlags);

    if (!pScene) {
        throw std::runtime_error("model load error! :: check model file - " + std::string(s_importer->GetErrorString()));
    }

    auto pRigidMeshRenderer = std::make_unique<RigidMeshRenderer>();
    RigidMesh rMesh;

    std::vector<Mesh> meshes;
    std::vector<UINT> matIndices;
    std::vector<UINT> boneIndices;
    std::vector<RigidBone> rigidBones;

    std::unordered_map <std::string, UINT> nodeNameToIndex;

    ProcessNode(-1, rigidBones, meshes, matIndices, boneIndices, pScene->mRootNode, pScene, nodeNameToIndex);
    rMesh.SetSubMesh(std::move(meshes));
    rMesh.SetMatIdx(std::move(matIndices));
    rMesh.SetBones(std::move(rigidBones));
    rMesh.SetBoneIndices(std::move(boneIndices));
    pRigidMeshRenderer->SetMesh(std::move(rMesh));


    if (pScene->HasAnimations())
    {
        std::vector<std::unordered_map<UINT, AnimationClip>> boneAnimations;
        boneAnimations.resize(pScene->mNumAnimations);

        for (UINT i = 0; i < pScene->mNumAnimations; i++)
        {
            auto& animation = pScene->mAnimations[i];
            //std::cout << animation->mName.C_Str() << std::endl;
            for (UINT j = 0; j < animation->mNumChannels; j++)
            {
                auto& animNode = animation->mChannels[j];

                //std::cout << "    ->" << animNode->mNodeName.C_Str() << std::endl;

                //if (nodeNameToIndex.find({ animNode->mNodeName.C_Str() }) != nodeNameToIndex.end())
                //    std::cout << "    nameMatching!" << std::endl;

                auto currentNodeIdx = nodeNameToIndex[{ animNode->mNodeName.C_Str() }];
                auto& currentAnimationClip = boneAnimations[i][currentNodeIdx];
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

        pRigidMeshRenderer->SetAnimations(std::move(boneAnimations));
    }


    for (UINT i = 0; i < pScene->mNumMaterials; i++)
    {
        pRigidMeshRenderer->AddMaterial(ProcessMaterial(pScene->mMaterials[i], pScene, BoneType::RigidBone));
    }

    return pRigidMeshRenderer;
}

std::unique_ptr<MyEngine::StaticMeshRenderer> MyEngine::AssimpConverter::LoadStaticMeshRendererFromFile(std::string filePath)
{
    //importFlag 세팅
    s_importFlags = aiProcess_Triangulate |    // vertex 삼각형 으로 출력
        aiProcess_GenNormals |        // Normal 정보 생성  
        aiProcess_GenUVCoords |      // 텍스처 좌표 생성
        aiProcess_CalcTangentSpace |  // 탄젠트 벡터 생성
        aiProcess_JoinIdenticalVertices |  // 중복 정점 제거
        aiProcess_ValidateDataStructure | // 구조 검증
	    //aiProcess_ConvertToLeftHanded |  // DX용 왼손좌표계 변환 <- 제외사유 : SimpleMath로 구현한 트랜스폼 클래스 때문에 이미 오른손좌표계임
	    aiProcess_PreTransformVertices |  // 노드의 변환행렬을 적용한 버텍스 생성한다.  *StaticMesh로 처리할때만
        aiProcess_FlipUVs; 


    Material::InitBlinnPhongShaders(s_pDevice);

    const aiScene* pScene = s_importer->ReadFile(filePath.c_str(), s_importFlags);

    if (!pScene) {
        throw std::runtime_error("model load error! :: check model file - " + std::string(s_importer->GetErrorString()));
    }

    auto pStaticMeshRenderer = std::make_unique<StaticMeshRenderer>();
    StaticMesh sMesh;

    std::vector<Mesh> meshes;
    std::vector<UINT> indices;

    ProcessNode(meshes, indices, pScene->mRootNode, pScene);
    sMesh.SetSubMesh(std::move(meshes));
    sMesh.SetMatIdx(std::move(indices));
    sMesh.CalcAABB();
    pStaticMeshRenderer->SetMesh(std::move(sMesh));

    for (UINT i = 0; i < pScene->mNumMaterials; i++)
    {
        pStaticMeshRenderer->AddMaterial(ProcessMaterial(pScene->mMaterials[i], pScene, BoneType::None));
    }

    return pStaticMeshRenderer;
}

std::unique_ptr<MyEngine::SkinningMeshRenderer> MyEngine::AssimpConverter::LoadSkinningMeshRendererFromFile(std::string filePath)
{
    // importFlag 세팅
    s_importFlags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs;

    // _$AssimpFbx$Translation, _$AssimpFbx$_PreRotation, _$AssimpFbx$_Rotation 등의 노드 분기 생성 방지
    s_importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    Material::InitBlinnPhongShaders(s_pDevice);

    const aiScene* pScene = s_importer->ReadFile(filePath.c_str(), s_importFlags);

    if (!pScene) {
        throw std::runtime_error("model load error! :: check model file - " + std::string(s_importer->GetErrorString()));
    }

    auto usedBones = CollectUsedBoneNames(pScene);
    std::unordered_set<std::string> boneHierarchy;
    CollectBoneHierarchy(pScene->mRootNode, usedBones, boneHierarchy);

    auto pSkinningMeshRenderer = std::make_unique<SkinningMeshRenderer>();
    SkinningMesh sMesh;

    std::vector<Mesh> meshes;
    std::vector<UINT> matIndices;
    std::vector<SkinningBone> skinningBones;
    std::vector<CorrectionNode> correctionMap;

    std::unordered_map<std::string, UINT> nodeNameToIndex;
    ProcessNode(-1, skinningBones, meshes, matIndices, pScene->mRootNode, pScene, nodeNameToIndex, correctionMap, boneHierarchy);

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
        allMeshBoneData[i].resize(meshes[i].GetVertices().size());
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
        auto& currentMeshVertices = meshes[meshIdx].GetVertices();
        auto& meshBoneData = allMeshBoneData[meshIdx];

        for (UINT vertIdx = 0; vertIdx < currentMeshVertices.size(); ++vertIdx)
        {
            auto& vertex = currentMeshVertices[vertIdx];
            auto& boneData = meshBoneData[vertIdx].boneWeights;

            // 1. 가중치 순으로 내림차순 정렬하여 가장 큰 4개의 뼈대만 선택
            std::sort(boneData.begin(), boneData.end(), [](const auto& a, const auto& b) {
                return a.second > b.second;
                });

            float totalWeight = 0.0f;

            // 2. 최대 4개의 뼈대 인덱스와 가중치를 정점 구조체에 할당
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

            // 3. 가중치 정규화 (선택: 합이 1.0이 되도록)
            if (totalWeight > 0.0f)
            {
                float factor = 1.0f / totalWeight;
                for (int i = 0; i < 4; ++i)
                {
                    vertex.boneWeights[i] *= factor;
                }
            }

            // 4. 초기화되지 않은 나머지 배열 요소는 0으로 보장 (VertexType 정의 시 초기화하는 것이 좋음)
        }
    }

    sMesh.SetSubMesh(std::move(meshes));
    sMesh.SetMatIdx(std::move(matIndices));
    sMesh.SetBones(std::move(skinningBones));
    pSkinningMeshRenderer->SetMesh(std::move(sMesh));


    if (pScene->HasAnimations())
    {
        std::vector<std::unordered_map<UINT, AnimationClip>> boneAnimations;
        boneAnimations.resize(pScene->mNumAnimations);

        for (UINT i = 0; i < pScene->mNumAnimations; i++)
        {
            auto& animation = pScene->mAnimations[i];
            //std::cout << animation->mName.C_Str() << std::endl;
            for (UINT j = 0; j < animation->mNumChannels; j++)
            {
                auto& animNode = animation->mChannels[j];

                //std::cout << "    ->" << animNode->mNodeName.C_Str() << std::endl;

                //if (nodeNameToIndex.find({ animNode->mNodeName.C_Str() }) != nodeNameToIndex.end())
                //    std::cout << "    nameMatching!" << std::endl;

                auto currentNodeIdx = nodeNameToIndex[{ animNode->mNodeName.C_Str() }];
                auto& currentAnimationClip = boneAnimations[i][currentNodeIdx];
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

        pSkinningMeshRenderer->SetAnimations(std::move(boneAnimations));
    }


    for (UINT i = 0; i < pScene->mNumMaterials; i++)
    {
        pSkinningMeshRenderer->AddMaterial(ProcessMaterial(pScene->mMaterials[i], pScene, BoneType::SkinningBone));
    }

    return pSkinningMeshRenderer;
}

MyEngine::AssimpConverter::LoadMaterialType MyEngine::AssimpConverter::s_materialType = MyEngine::AssimpConverter::LoadMaterialType::BlinnPhong;

void MyEngine::AssimpConverter::SetLoadMaterialType(LoadMaterialType type)
{
    s_materialType = type;
}
