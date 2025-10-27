#include "StaticMeshRenderer.h"

int DebugStatusUI::StaticMeshRenderer::meshNum = 1;
bool DebugStatusUI::StaticMeshRenderer::limitDrawOption = false;

void MyEngine::StaticMeshRenderer::Draw(ID3D11DeviceContext* context, bool bindMesh, bool bindMaterial)
{
    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    int matCount = 0;
    int drawCount = 0;

    for (auto& mesh : m_staticMesh.GetMeshes())
    {
        if(bindMesh)
            mesh.Bind(context);
        auto& materialIndices = m_staticMesh.GetMaterialIndices();
        auto& mat = m_materials[materialIndices[matCount++]];
        if (bindMaterial)
        {
            mat.Bind(context);
        }
        else
        {
            context->VSSetShader(mat.GetVertexShader(), nullptr, 0);
        }
        drawCount++;
     
        if (DebugStatusUI::StaticMeshRenderer::limitDrawOption 
            && (drawCount > DebugStatusUI::StaticMeshRenderer::meshNum 
            || drawCount <= DebugStatusUI::StaticMeshRenderer::meshNum - 1))
            continue;
        context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
    }
}
