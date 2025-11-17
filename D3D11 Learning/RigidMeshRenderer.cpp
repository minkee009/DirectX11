#include <algorithm>
#include <iostream>

#include "RigidMeshRenderer.h"
#include "StaticMeshRenderer.h"
#include "TimeManager.h"

MyEngine::RigidMeshRenderer::RigidMeshRenderer()
{
	m_pBoneMatrixData = std::make_unique<RigidBoneMatCB>();
}

void MyEngine::RigidMeshRenderer::Draw(ID3D11DeviceContext* context)
{
	if (!m_boneMatrixCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(RigidBoneMatCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneMatrixCB.GetAddressOf());
		pDevice->Release();
		if (FAILED(hr))
			return;
	}

	if (!m_boneMatrixIdxCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(RigidBoneMatIdxCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneMatrixIdxCB.GetAddressOf());
		pDevice->Release();
		if (FAILED(hr))
			return;
	}

	context->UpdateSubresource(m_boneMatrixCB.Get(), 0, nullptr, m_pBoneMatrixData.get(), 0, 0);
	context->VSSetConstantBuffers(2, 1, m_boneMatrixCB.GetAddressOf());

	UINT stride = sizeof(DefaultVertex);
	UINT offset = 0;

	int meshCount = 0;
	auto& boneIndices = m_pRigidMesh->GetBoneIndices();

	auto& materialIndices = GetMatRefIndices();

	for (auto& mesh : m_pRigidMesh->GetMeshes())
	{
		RigidBoneMatIdxCB cb2;
		if(GetEnabledBindMeshes())
			mesh.Bind(context);
		cb2.index = boneIndices[meshCount];

		context->UpdateSubresource(m_boneMatrixIdxCB.Get(), 0, nullptr, &cb2, 0, 0);
		context->VSSetConstantBuffers(3, 1, m_boneMatrixIdxCB.GetAddressOf());

		auto& mat = m_materials[materialIndices[meshCount++]];
		if (GetEnabledBindMaterials())
		{
			mat->Bind(context);
		}
		auto passForceVSIter = GetPassForceChangeVS().find(GetRenderPassNum());
		if (passForceVSIter != GetPassForceChangeVS().end())
			context->VSSetShader(passForceVSIter->second, nullptr, 0);
		auto passForcePSIter = GetPassForceChangePS().find(GetRenderPassNum());
		if (passForcePSIter != GetPassForceChangePS().end())
			context->PSSetShader(passForcePSIter->second, nullptr, 0);

		if (DebugStatusUI::MeshRenderer::limitDrawOption
			&& (meshCount > DebugStatusUI::MeshRenderer::meshNum
				|| meshCount <= DebugStatusUI::MeshRenderer::meshNum - 1))
			continue;

		auto passIter = GetPassExcludedMeshes().find(GetRenderPassNum());
		if (passIter != GetPassExcludedMeshes().end()
			&& passIter->second.find(meshCount) != passIter->second.end())
			continue;

		context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
	}
}
void MyEngine::RigidMeshRenderer::MatrixUpdate()
{
	auto& bones = m_pRigidMesh->GetBones();
	for (UINT i = 0; i < bones.size(); i++)
	{
		auto& bonePose = m_bonePoses[i];
		auto& bone = bones[i];

		if (bone.parentIndex != -1)
		{
			bonePose.model = m_bonePoses[bone.parentIndex].model * bonePose.local;
		}
		else
		{
			bonePose.model = bonePose.local;
		}

		if (m_pBoneMatrixData)
			m_pBoneMatrixData->matricies[i] = bonePose.model;
	}
}

void MyEngine::RigidMeshRenderer::AnimationUpdate()
{
	if (m_boneAnimations.empty() || !m_playing)
		return;

	auto& anim = m_boneAnimations[m_animationIdx];
	auto& bones = m_pRigidMesh->GetBones();
	auto& duration = anim.begin()->second.duration;

	m_time += TIME_GET_DELTA() * m_speed;
	m_time = std::fmod(m_time, duration);

	for (auto& pair : anim)
	{
		auto& index = pair.first;
		auto& clip = pair.second;
		auto& bone = bones[index];

		auto actualTime = m_time * clip.frameRate;

		Vector3 currentPos = clip.pos.Evaluate(actualTime);
		Quaternion currentRot = clip.rot.Evaluate(actualTime);
		Vector3 currentScale = clip.scale.Evaluate(actualTime);

		Matrix S = Matrix::CreateScale(currentScale);

		Matrix R = Matrix::CreateFromQuaternion(currentRot);

		Matrix T = Matrix::CreateTranslation(currentPos);

		m_bonePoses[bone.index].local = S * R * T;
		m_bonePoses[bone.index].local = m_bonePoses[bone.index].local.Transpose();
	}
}

void MyEngine::RigidMeshRenderer::Play()
{
	m_playing = true;
}

void MyEngine::RigidMeshRenderer::Pause()
{
	m_playing = false;
}

