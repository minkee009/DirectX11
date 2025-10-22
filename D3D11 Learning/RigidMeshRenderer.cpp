#include "RigidMeshRenderer.h"
#include "StaticMeshRenderer.h"

void MyEngine::RigidMeshRenderer::Draw(ID3D11DeviceContext* context)
{
	if (!m_boneMatCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(BoneMatCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneMatCB.GetAddressOf());
		if (FAILED(hr))
			return;
	}

	if (!m_boneMatIdxCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(BoneMatIdxCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneMatIdxCB.GetAddressOf());
		if (FAILED(hr))
			return;
	}

	//상수버퍼에 올릴 model버퍼 연산 및 집어넣기

	BoneMatCB cb1;
	auto& bones = m_rigidMesh.GetBones();

	for (UINT i = 0; i < bones.size(); i++)
	{
		auto& bone = bones[i];
		
		if (bone.parentIndex != -1)
		{
			auto& boneParent = bones[bone.parentIndex];
			bone.model = boneParent.model * bone.local;
		}
		else
		{
			bone.model = bone.local;
		}

		cb1.matricies[i] = bone.model;
	}
	context->UpdateSubresource(m_boneMatCB.Get(), 0, nullptr, &cb1, 0, 0);
	context->VSSetConstantBuffers(2, 1, m_boneMatCB.GetAddressOf());

	UINT stride = sizeof(VertexType);
	UINT offset = 0;

	int meshCount = 0;
	for (auto& mesh : m_rigidMesh.GetMeshes())
	{
		BoneMatIdxCB cb2;

		mesh.Bind(context);
		cb2.index = meshCount + 1;

		context->UpdateSubresource(m_boneMatIdxCB.Get(), 0, nullptr, &cb2, 0, 0);
		context->VSSetConstantBuffers(3, 1, m_boneMatIdxCB.GetAddressOf());

		auto& materialIndices = m_rigidMesh.GetMaterialIndices();
		m_materials[materialIndices[meshCount++]].Bind(context);

		if (DebugStatusUI::StaticMeshRenderer::limitDrawOption
			&& (meshCount > DebugStatusUI::StaticMeshRenderer::meshNum
				|| meshCount <= DebugStatusUI::StaticMeshRenderer::meshNum - 1))
			continue;

		context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
	}
}
