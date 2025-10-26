#include "StaticMeshRenderer.h"

int DebugStatusUI::StaticMeshRenderer::meshNum = 4;
bool DebugStatusUI::StaticMeshRenderer::limitDrawOption = false;

void MyEngine::StaticMeshRenderer::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    int matCount = 0;
    int drawCount = 0;

    //16바이트 버퍼전송 -> UseBoneMatrix 항목만 업데이트
    //...코드

    for (auto& mesh : m_staticMesh.GetMeshes())
    {
        mesh.Bind(context);
        auto& materialIndices = m_staticMesh.GetMaterialIndices();
        m_materials[materialIndices[matCount++]].Bind(context);
        drawCount++;
     
        if (DebugStatusUI::StaticMeshRenderer::limitDrawOption 
            && (drawCount > DebugStatusUI::StaticMeshRenderer::meshNum 
            || drawCount <= DebugStatusUI::StaticMeshRenderer::meshNum - 1))
            continue;
        context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
    }
}
