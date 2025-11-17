#include <algorithm>
#include <iostream>

#include "SkinningMeshRenderer.h"
#include "StaticMeshRenderer.h"
#include "TimeManager.h"

MyEngine::SkinningMeshRenderer::SkinningMeshRenderer()
{
	m_pBoneModelMatrixData = std::make_unique<SkinningBoneMatCB>();
}

MyEngine::SkinningMeshRenderer::~SkinningMeshRenderer()
{
	m_pBoneModelMatrixCB = nullptr;
}

void MyEngine::SkinningMeshRenderer::Draw(ID3D11DeviceContext* context)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	context->Map(m_pBoneModelMatrixCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, m_pBoneModelMatrixData.get(), sizeof(SkinningBoneMatCB));
	context->Unmap(m_pBoneModelMatrixCB.Get(), 0);

	context->VSSetConstantBuffers(2, 1, m_pBoneModelMatrixCB.GetAddressOf());
	context->VSSetConstantBuffers(3, 1, m_pSkinningMesh->GetBoneOffsetMatirxBufferAddress());

	UINT stride = sizeof(DefaultVertex);
	UINT offset = 0;
	auto& materialIndices = GetMatRefIndices();

	int meshCount = 0;
	for (auto& mesh : m_pSkinningMesh->GetMeshes())
	{
		if(GetEnabledBindMeshes())
			mesh.Bind(context);

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

void MyEngine::SkinningMeshRenderer::CalcBBox()
{
	bool firstBone = true;

	auto& bones = m_pSkinningMesh->GetBones();

	for (auto& bone : bones)
	{
		if (bone.parentIndex == -1)
			continue;

		if (bone.hasVertex)
		{
			auto& boneModelMat = m_bonePoses[bone.index].model;

			BoundingBox transformed;
			bone.bbox.Transform(transformed, boneModelMat.Transpose());

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

void MyEngine::SkinningMeshRenderer::CreateBoneModelMatrixBuffer(ID3D11DeviceContext* context)
{
	if (!m_pBoneModelMatrixCB)
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

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_pBoneModelMatrixCB.GetAddressOf());
		pDevice->Release();
		if (FAILED(hr))
			return;
	}
}

void MyEngine::SkinningMeshRenderer::MatrixUpdate()
{
	auto& bones = m_pSkinningMesh->GetBones();
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

		if(m_pBoneModelMatrixData)
			m_pBoneModelMatrixData->matricies[i] = bonePose.model;
	}

	CalcBBox();
}

void MyEngine::SkinningMeshRenderer::AnimationUpdate()
{
	if (m_boneAnimations.empty() || !m_playing)
		return;

	auto& anim = m_boneAnimations[m_animationIdx];
	auto& bones = m_pSkinningMesh->GetBones();
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

void MyEngine::SkinningMeshRenderer::Play()
{
	m_playing = true;
}

void MyEngine::SkinningMeshRenderer::Pause()
{
	m_playing = false;
}

MyEngine::SkinningMesh::SkinningMesh()
{
	m_pBoneOffsetMatrixData = std::make_unique<SkinningBoneMatCB>();
}

MyEngine::SkinningMesh::~SkinningMesh()
{
	m_pBoneOffsetMatrixData = nullptr;
}

MyEngine::SkinningMesh::SkinningMesh(SkinningMesh&& other) noexcept 
	: StaticMesh(std::move(other))
	, m_bones(std::move(other.m_bones))
	, m_pBoneOffsetMatrixData(std::move(other.m_pBoneOffsetMatrixData))
	, m_pBoneOffsetMatrixCB(std::move(other.m_pBoneOffsetMatrixCB))
{

}

MyEngine::SkinningMesh& MyEngine::SkinningMesh::operator=(SkinningMesh&& other) noexcept
{
	if (this != &other)
	{
		m_bones = std::move(other.m_bones);
		m_pBoneOffsetMatrixData = std::move(other.m_pBoneOffsetMatrixData);
		m_pBoneOffsetMatrixCB = std::move(other.m_pBoneOffsetMatrixCB);
	}
	return *this;
}

void MyEngine::SkinningMesh::CreateBoneOffsetMatrixBuffer(ID3D11DeviceContext* context)
{
	if (!m_pBoneOffsetMatrixCB)
	{
		//본 offset 행렬은 최초 1회 초기화
		auto& bones = m_bones;
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

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, &initData, m_pBoneOffsetMatrixCB.GetAddressOf());
		pDevice->Release();
		if (FAILED(hr))
			return;
	}
}
