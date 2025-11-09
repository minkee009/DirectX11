#include "StaticMeshRenderer.h"

void MyEngine::StaticMeshRenderer::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    int matCount = 0;
    int drawCount = 0;

    for (auto& mesh : m_staticMesh.GetMeshes())
    {
        if(GetEnabledBindMeshes())
            mesh.Bind(context);
        auto& materialIndices = m_staticMesh.GetMaterialIndices();
        auto& mat = m_materials[materialIndices[matCount++]];
        if (GetEnabledBindMaterials())
        {
            mat.Bind(context);
        }
        auto passForceVSIter = GetPassForceChangeVS().find(GetRenderPassNum());
        if (passForceVSIter != GetPassForceChangeVS().end())
            context->VSSetShader(passForceVSIter->second, nullptr, 0);
        auto passForcePSIter = GetPassForceChangePS().find(GetRenderPassNum());
        if (passForcePSIter != GetPassForceChangePS().end())
            context->PSSetShader(passForcePSIter->second, nullptr, 0);

        drawCount++;
     
        if (DebugStatusUI::MeshRenderer::limitDrawOption
            && (drawCount > DebugStatusUI::MeshRenderer::meshNum
            || drawCount <= DebugStatusUI::MeshRenderer::meshNum - 1))
            continue;
        auto passExcludeIter = GetPassExcludedMeshes().find(GetRenderPassNum());
        if (passExcludeIter != GetPassExcludedMeshes().end()
            && passExcludeIter->second.find(drawCount) != passExcludeIter->second.end())
            continue;

        context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
    }
}
