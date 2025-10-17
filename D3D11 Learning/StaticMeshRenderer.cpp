#include "StaticMeshRenderer.h"

void MyEngine::StaticMeshRenderer::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(VertexType);
    UINT offset = 0;

    //Material::BindDefaultShaders(context);
    int matCount = 0;
    for (auto& mesh : m_staticMesh.GetMeshes())
    {
        mesh.Bind(context);
        auto& materialIndices = m_staticMesh.GetMaterialIndices();
        m_materials[materialIndices[matCount++]].Bind(context);
        context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
    }
}
