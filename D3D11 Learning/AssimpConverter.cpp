#include "AssimpConverter.h"
#include <stdexcept>
#include <filesystem>

std::unique_ptr<Assimp::Importer> MyEngine::AssimpConverter::s_importer = nullptr;
uint32_t MyEngine::AssimpConverter::s_importFlags = 0;
ID3D11Device* MyEngine::AssimpConverter::s_pDevice = nullptr;
ID3D11DeviceContext* MyEngine::AssimpConverter::s_pContext = nullptr;

namespace fs = std::filesystem;

void MyEngine::AssimpConverter::ProcessNode(std::vector<Mesh>& meshes,std::vector<UINT>& matIDX, aiNode* pNode, const aiScene* pScene)
{
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        meshes.push_back(ProcessMesh(meshes, pMesh, pScene));
        matIDX.push_back(pMesh->mMaterialIndex);
    }

    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        ProcessNode(meshes, matIDX, pNode->mChildren[i], pScene);
    }
}

MyEngine::Mesh MyEngine::AssimpConverter::ProcessMesh(std::vector<Mesh>& meshes, aiMesh* pMesh, const aiScene* pScene)
{
    std::vector<VertexType> vertices;
    std::vector<UINT> indices;

    for (UINT i = 0; i < pMesh->mNumVertices; i++)
    {
        VertexType vertex;

        vertex.position = { pMesh->mVertices[i].x,pMesh->mVertices[i].y,pMesh->mVertices[i].z };

        vertex.normal = { pMesh->mNormals[i].x,pMesh->mNormals[i].y,pMesh->mNormals[i].z};
        vertex.tangent = { pMesh->mTangents[i].x, pMesh->mTangents[i].y ,pMesh->mTangents[i].z };
        if (pMesh->mTextureCoords[0])
        {
            vertex.uv = { (float)pMesh->mTextureCoords[0][i].x, 1.0f - (float)pMesh->mTextureCoords[0][i].y };
        }
        vertices.push_back(vertex);
    }

    for (UINT i = 0; i < pMesh->mNumFaces; i++) {
        aiFace face = pMesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    return Mesh(vertices, indices, s_pDevice);
}

MyEngine::Material MyEngine::AssimpConverter::ProcessMaterial(aiScene* pScene, aiMaterial* pMat)
{
    Material mat{ { pMat->GetName().C_Str()} };

    mat.InitSampler(s_pDevice);
    mat.InitShader(ShaderType::Vertex, Material::GetBlinnPhongVertexShader(), Material::GetBlinnPhongVSBlob());
    mat.InitShader(ShaderType::Pixel, Material::GetBlinnPhongPixelShader(), nullptr);

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
    s_importFlags = aiProcess_Triangulate |    // vertex 삼각형 으로 출력
        aiProcess_GenNormals |        // Normal 정보 생성  
        aiProcess_GenUVCoords |      // 텍스처 좌표 생성
        aiProcess_CalcTangentSpace |  // 탄젠트 벡터 생성
        aiProcess_JoinIdenticalVertices |  // 중복 정점 제거
        aiProcess_ValidateDataStructure; // 구조 검증

    Material::InitBlinnPhongShaders(s_pDevice);

    const aiScene* pScene = s_importer->ReadFile(filePath.c_str(), s_importFlags);

    return std::unique_ptr<RigidMeshRenderer>();
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
	    aiProcess_PreTransformVertices;  // 노드의 변환행렬을 적용한 버텍스 생성한다.  *StaticMesh로 처리할때만


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
    pStaticMeshRenderer->SetMesh(std::move(sMesh));

    for (UINT i = 0; i < pScene->mNumMaterials; i++)
    {
        pStaticMeshRenderer->AddMaterial(ProcessMaterial(pScene->mMaterials[i]));
    }
    pStaticMeshRenderer->AddMaterial(ProcessMaterial(pScene->mMaterials[0]));

    return pStaticMeshRenderer;
}
