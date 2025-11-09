#include "MeshRenderer.h"

int DebugStatusUI::MeshRenderer::meshNum = 0;
bool DebugStatusUI::MeshRenderer::limitDrawOption = false;


void MyEngine::MeshRenderer::SetPassExcludedMeshes(UINT renderPassNum, std::initializer_list<UINT> meshes)
{
	m_passExcludedMeshes[renderPassNum] = meshes;
}

void MyEngine::MeshRenderer::SetPassForceChangeVS(UINT renderPassNum, ID3D11VertexShader* VS)
{
	m_passForceChangeVS[renderPassNum] = VS;
}

void MyEngine::MeshRenderer::SetPassForceChangePS(UINT renderPassNum, ID3D11PixelShader* PS)
{
	m_passForceChangePS[renderPassNum] = PS;
}
