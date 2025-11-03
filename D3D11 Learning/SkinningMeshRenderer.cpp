#include <algorithm>
#include <iostream>

#include "SkinningMeshRenderer.h"
#include "StaticMeshRenderer.h"
#include "Time.h"

MyEngine::SkinningMeshRenderer::SkinningMeshRenderer()
{
	m_pBoneModelMatrixData = std::make_unique<SkinningBoneMatCB>();
	m_pBoneOffsetMatrixData = std::make_unique<SkinningBoneMatCB>();
}

void MyEngine::SkinningMeshRenderer::Draw(ID3D11DeviceContext* context, bool bindMesh, bool bindMaterial)
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

		//본 offset 행렬은 최초 1회 초기화
		auto& bones = m_skinningMesh.GetBones();
		for (size_t i = 0; i < bones.size(); i++)
		{
			m_pBoneOffsetMatrixData->matricies[i] = bones[i].offset;
		}
		context->UpdateSubresource(m_boneOffsetMatrixCB.Get(), 0, nullptr, m_pBoneOffsetMatrixData.get(), 0, 0);
	}

	context->UpdateSubresource(m_boneModelMatrixCB.Get(), 0, nullptr, m_pBoneModelMatrixData.get(), 0, 0);

	context->VSSetConstantBuffers(2, 1, m_boneModelMatrixCB.GetAddressOf());
	context->VSSetConstantBuffers(3, 1, m_boneOffsetMatrixCB.GetAddressOf());

	UINT stride = sizeof(VertexType);
	UINT offset = 0;

	int meshCount = 0;
	for (auto& mesh : m_skinningMesh.GetMeshes())
	{
		if(bindMesh)
			mesh.Bind(context);

		auto& materialIndices = m_skinningMesh.GetMaterialIndices();
		auto& mat = m_materials[materialIndices[meshCount++]];
		if (bindMaterial)
		{
			mat.Bind(context);
		}

		if (DebugStatusUI::StaticMeshRenderer::limitDrawOption
			&& (meshCount > DebugStatusUI::StaticMeshRenderer::meshNum
				|| meshCount <= DebugStatusUI::StaticMeshRenderer::meshNum - 1))
			continue;

		context->DrawIndexed(static_cast<UINT>(mesh.GetIndices().size()), 0, 0);
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

	m_skinningMesh.CalcAABB();
}

void MyEngine::SkinningMeshRenderer::AnimationUpdate()
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

void MyEngine::SkinningMesh::CalcAABB()
{
	m_aabb.min.x = FLT_MAX;
	m_aabb.min.y = FLT_MAX;
	m_aabb.min.z = FLT_MAX;

	m_aabb.max.x = -FLT_MAX;
	m_aabb.max.y = -FLT_MAX;
	m_aabb.max.z = -FLT_MAX;

	for (auto& bone : m_bones)
	{
		if (bone.boundBox.min.x == FLT_MAX || bone.parentIndex == -1) continue;

		auto& bbox = bone.boundBox;
		auto corners = bbox.ExtractCorners();

		for (size_t i = 0; i < corners.size(); i++)
		{
			corners[i] = Vector3::Transform(corners[i], bone.model.Transpose());

			if (m_aabb.min.x > corners[i].x)
				m_aabb.min.x = corners[i].x;
			if (m_aabb.min.y > corners[i].y)
				m_aabb.min.y = corners[i].y;
			if (m_aabb.min.z > corners[i].z)
				m_aabb.min.z = corners[i].z;

			if (m_aabb.max.x < corners[i].x)
				m_aabb.max.x = corners[i].x;
			if (m_aabb.max.y < corners[i].y)
				m_aabb.max.y = corners[i].y;
			if (m_aabb.max.z < corners[i].z)
				m_aabb.max.z = corners[i].z;
		}
	}
}
