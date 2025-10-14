#include "FBXSceneGraph.h"
#include <stdexcept>

std::unique_ptr<Assimp::Importer> MyEngine::AssimpConverter::s_importer = nullptr;
uint32_t MyEngine::AssimpConverter::s_importFlags = 0;
ID3D11Device* MyEngine::AssimpConverter::s_pDevice = nullptr;

void MyEngine::AssimpConverter::ProcessNode(std::vector<Mesh>& meshes, aiNode* pNode, const aiScene* pScene)
{
    for (UINT i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
        meshes.push_back(ProcessMesh(meshes, pMesh, pScene));
    }

    for (UINT i = 0; i < pNode->mNumChildren; i++)
    {
        ProcessNode(meshes, pNode->mChildren[i], pScene);
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

std::unique_ptr<MyEngine::FBXSceneGraph> MyEngine::AssimpConverter::LoadFromFile(std::string filePath)
{
    const aiScene* pScene = s_importer->ReadFile(filePath.c_str(), s_importFlags);

    if (!pScene) {
        throw std::runtime_error("fbx load error! :: check fbx file");
    }

    auto pSceneGraph = std::make_unique<FBXSceneGraph>();

    ProcessNode(pSceneGraph->m_meshes, pScene->mRootNode, pScene);

    return pSceneGraph;
}

void MyEngine::FBXSceneGraph::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    int meshCount = 0;

    for (auto& mesh : m_meshes)
    {
        /*if (meshCount == 1 || meshCount == 3)
            continue;*/

        auto vbp = mesh.GetVertexBuffer();
        auto ibp = mesh.GetIndexBuffer();

        context->IASetVertexBuffers(0, 1, &vbp, &stride, &offset);
        context->IASetIndexBuffer(ibp, DXGI_FORMAT_R32_UINT, 0);

        //context->PSSetShaderResources(0, 1, &textures_[0].texture);

        context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
        meshCount++;
    }
}
