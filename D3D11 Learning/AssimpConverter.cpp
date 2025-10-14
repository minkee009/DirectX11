#include "AssimpConverter.h"
#include <stdexcept>


#include <iostream>

std::unique_ptr<Assimp::Importer> MyEngine::AssimpConverter::s_importer = nullptr;
uint32_t MyEngine::AssimpConverter::s_importFlags = 0;
ID3D11Device* MyEngine::AssimpConverter::s_pDevice = nullptr;

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
        //vertex.binormal = { pMesh->mBitangents[i].x, pMesh->mBitangents[i].y,  pMesh->mBitangents[i].z };
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

MyEngine::Material MyEngine::AssimpConverter::ProcessMaterial(aiMaterial* pMat)
{
    Material mat{ { pMat->GetName().C_Str()} };

    mat.InitSampler(s_pDevice);
    mat.InitShader(ShaderType::Vertex, Material::GetBlinnPhongVertexShader(), Material::GetBlinnPhongVSBlob());
    mat.InitShader(ShaderType::Pixel, Material::GetBlinnPhongPixelShader(), nullptr);

    for (UINT i = 0; i < pMat->mNumProperties; i++)
    {
        std::cout << pMat->mProperties[i]->mData << std::endl;
        std::cout << pMat->mProperties[i]->mDataLength << std::endl;
        std::cout << pMat->mProperties[i]->mIndex << std::endl;
        std::cout << pMat->mProperties[i]->mSemantic << std::endl;
        std::cout << pMat->mProperties[i]->mKey.C_Str() << std::endl;
        std::cout << pMat->mProperties[i]->mType << std::endl;
    }

    return mat;
}

void MyEngine::AssimpConverter::Initialize(ID3D11Device* device)
{
    s_pDevice = device;
    s_importer = std::make_unique<Assimp::Importer>();
    s_importFlags = aiProcess_Triangulate |    // vertex 삼각형 으로 출력
        aiProcess_GenNormals |        // Normal 정보 생성  
        aiProcess_GenUVCoords |      // 텍스처 좌표 생성
        aiProcess_CalcTangentSpace |  // 탄젠트 벡터 생성
        //aiProcess_ConvertToLeftHanded |  // DX용 왼손좌표계 변환 <- 제외사유 : SimpleMath로 구현한 트랜스폼 클래스 때문에 오른손좌표계임
        aiProcess_PreTransformVertices;  // 노드의 변환행렬을 적용한 버텍스 생성한다.  *StaticMesh로 처리할때만
}

void MyEngine::AssimpConverter::Release()
{
    s_importer.reset();
}

std::unique_ptr<MyEngine::FBXSceneGraph> MyEngine::AssimpConverter::LoadSceneGraphFromFile(std::string filePath)
{
    Material::InitBlinnPhongShaders(s_pDevice);

    const aiScene* pScene = s_importer->ReadFile(filePath.c_str(), s_importFlags);

    if (!pScene) {
        throw std::runtime_error("fbx load error! :: check fbx file");
    }

    auto pSceneGraph = std::make_unique<FBXSceneGraph>();
    std::vector<UINT> m_matIDX;

    ProcessNode(pSceneGraph->m_meshes, m_matIDX, pScene->mRootNode, pScene);

    for (UINT i = 0; i < pScene->mNumMaterials; i++)
    {
        pSceneGraph->m_materials.push_back(ProcessMaterial(pScene->mMaterials[i]));
    }

    return pSceneGraph;
}

std::unique_ptr<MyEngine::StaticMeshRenderer> MyEngine::AssimpConverter::LoadStaticRendererFromFile(std::string filePath)
{
    return std::unique_ptr<StaticMeshRenderer>();
}

void MyEngine::FBXSceneGraph::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    //Material::BindDefaultShaders(context);
    for (auto& mesh : m_meshes)
    {
        mesh.Bind(context);
        //context->PSSetShaderResources(0, 1, &textures_[0].texture);

        context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
    }
}
