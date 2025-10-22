#pragma once
#include <vector>
#include "StaticMesh.h"
#include "Material.h"
#include "directxtk/SimpleMath.h"

using namespace SimpleMath;

namespace MyEngine
{
	struct BoneMatCB
	{
		Matrix matricies[128];
	};
	
	struct BoneMatIdxCB
	{
		UINT index = 0;
		float padding[3];
	};

	struct RigidBone
	{
		int index = -1;
		int parentIndex = -1;
		Matrix local; //상수 값 - 로드할 때 한번만 연산
		Matrix model; //프레임당 한번만 연산
	};

	class RigidMesh : public StaticMesh
	{
	private:
		std::vector<RigidBone> m_bones;
	public:
		void SetBones(std::vector<RigidBone>&& bones) { m_bones = std::move(bones); }
		inline std::vector<RigidBone>& GetBones() { return m_bones; }
	};

	class RigidMeshRenderer
	{
	private:
		RigidMesh m_rigidMesh;
		std::vector<Material> m_materials;
		// Bind -> 애니메이션 상수버퍼를 올려줌 ( matrix[128],matIdx )
		ComPtr<ID3D11Buffer> m_boneMatCB;
		ComPtr<ID3D11Buffer> m_boneMatIdxCB;
	public:
		inline void SetMesh(RigidMesh&& mesh) { m_rigidMesh = std::move(mesh); }
		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		void Draw(ID3D11DeviceContext* context);
	};
}