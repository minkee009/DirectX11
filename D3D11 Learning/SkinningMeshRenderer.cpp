#include <algorithm>
#include <iostream>

#include "SkinningMeshRenderer.h"
#include "StaticMeshRenderer.h"
#include "TimeManager.h"

MyEngine::SkinningMeshRenderer::SkinningMeshRenderer()
{
	m_pBoneModelMatrixData = std::make_unique<SkinningBoneMatCB>();
	m_pBoneOffsetMatrixData = std::make_unique<SkinningBoneMatCB>();
}

MyEngine::SkinningMeshRenderer::~SkinningMeshRenderer()
{
	m_boneModelMatrixCB = nullptr;
	m_boneOffsetMatrixCB = nullptr;
}

void MyEngine::SkinningMeshRenderer::Draw(ID3D11DeviceContext* context)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	context->Map(m_boneModelMatrixCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, m_pBoneModelMatrixData.get(), sizeof(SkinningBoneMatCB));
	context->Unmap(m_boneModelMatrixCB.Get(), 0);

	context->VSSetConstantBuffers(2, 1, m_boneModelMatrixCB.GetAddressOf());
	context->VSSetConstantBuffers(3, 1, m_boneOffsetMatrixCB.GetAddressOf());

	UINT stride = sizeof(DefaultVertex);
	UINT offset = 0;
	auto& materialIndices = GetMatRefIndices();

	int meshCount = 0;
	for (auto& mesh : m_skinningMesh.GetMeshes())
	{
		if(GetEnabledBindMeshes())
			mesh.Bind(context);

		auto& mat = m_materials[materialIndices[meshCount++]];
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

void MyEngine::SkinningMeshRenderer::CreateBoneMatrixBuffers(ID3D11DeviceContext* context)
{
	if (!m_boneModelMatrixCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.ByteWidth = sizeof(SkinningBoneMatCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneModelMatrixCB.GetAddressOf());
		pDevice->Release();
		if (FAILED(hr))
			return;
	}

	if (!m_boneOffsetMatrixCB)
	{
		//본 offset 행렬은 최초 1회 초기화
		auto& bones = m_skinningMesh.GetBones();
		for (size_t i = 0; i < bones.size(); i++)
		{
			m_pBoneOffsetMatrixData->matricies[i] = bones[i].offset;
		}

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = m_pBoneOffsetMatrixData.get();
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_IMMUTABLE;
		cbDesc.ByteWidth = sizeof(SkinningBoneMatCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, &initData, m_boneOffsetMatrixCB.GetAddressOf());
		pDevice->Release();
		if (FAILED(hr))
			return;
	}
}

void MyEngine::SkinningMeshRenderer::MatrixUpdate()
{
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

		if(m_pBoneModelMatrixData)
			m_pBoneModelMatrixData->matricies[i] = bone.model;
	}

	m_skinningMesh.CalcBBox();
}

void MyEngine::SkinningMeshRenderer::AnimationUpdate()
{
	if (m_boneAnimations.empty() || !m_playing)
		return;

	auto& anim = m_boneAnimations[m_animationIdx];
	auto& bones = m_skinningMesh.GetBones();
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

void MyEngine::SkinningMesh::CalcBBox()
{
	bool firstBone = true;

	for (auto& bone : m_bones)
	{
		if (bone.parentIndex == -1)
			continue;

		if (bone.hasVertex)
		{
			BoundingBox transformed;
			bone.bbox.Transform(transformed, bone.model.Transpose());

			if (firstBone)
			{
				m_bbox = transformed;  // 첫 번째는 직접 할당
				firstBone = false;
			}
			else
			{
				BoundingBox::CreateMerged(m_bbox, m_bbox, transformed);
			}
		}
	}
}
