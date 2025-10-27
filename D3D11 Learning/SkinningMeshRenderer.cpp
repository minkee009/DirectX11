#include <algorithm>
#include <iostream>

#include "SkinningMeshRenderer.h"
#include "StaticMeshRenderer.h"
#include "Time.h"

void MyEngine::SkinningMeshRenderer::Draw(ID3D11DeviceContext* context)
{
	if (!m_boneModelMatrixCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(SkinningBoneMatCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneModelMatrixCB.GetAddressOf());
		if (FAILED(hr))
			return;
	}

	if (!m_boneOffsetMatrixCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(SkinningBoneMatCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneOffsetMatrixCB.GetAddressOf());
		if (FAILED(hr))
			return;
	}

	//상수버퍼에 올릴 model버퍼 연산 및 집어넣기
	MatrixUpdate();
	if(!m_boneModelMatrixData)
		m_boneModelMatrixData = std::make_unique<SkinningBoneMatCB>();

	if (!m_boneOffsetMatrixData)
		m_boneOffsetMatrixData = std::make_unique<SkinningBoneMatCB>();

	auto& bones = m_skinningMesh.GetBones();

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

		m_boneModelMatrixData->matricies[i] = bone.model * bone.offset;
	}
	context->UpdateSubresource(m_boneModelMatrixCB.Get(), 0, nullptr, m_boneModelMatrixData.get(), 0, 0);
	context->UpdateSubresource(m_boneOffsetMatrixCB.Get(), 0, nullptr, m_boneOffsetMatrixData.get(), 0, 0);
	context->VSSetConstantBuffers(2, 1, m_boneModelMatrixCB.GetAddressOf());
	context->VSSetConstantBuffers(3, 1, m_boneOffsetMatrixCB.GetAddressOf());

	UINT stride = sizeof(VertexType);
	UINT offset = 0;

	int meshCount = 0;
	for (auto& mesh : m_skinningMesh.GetMeshes())
	{
		mesh.Bind(context);

		auto& materialIndices = m_skinningMesh.GetMaterialIndices();
		m_materials[materialIndices[meshCount++]].Bind(context);

		if (DebugStatusUI::StaticMeshRenderer::limitDrawOption
			&& (meshCount > DebugStatusUI::StaticMeshRenderer::meshNum
				|| meshCount <= DebugStatusUI::StaticMeshRenderer::meshNum - 1))
			continue;

		context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
	}
}

void MyEngine::SkinningMeshRenderer::MatrixUpdate()
{
	if (m_boneAnimations.empty() || !m_playing)
		return;

	auto& anim = m_boneAnimations[m_animationIdx];
	auto& bones = m_skinningMesh.GetBones();
	auto& duration = anim.begin()->second.duration;

	m_time += Time::instance->GetDeltaTime() * m_speed;
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

		bone.local = S * R * T;
		bone.local = bone.local.Transpose();
	}
}

void MyEngine::SkinningMeshRenderer::Play()
{
	m_playing = true;
}

void MyEngine::SkinningMeshRenderer::Pause()
{
	m_playing = false;
}

