#pragma once
#include <vector>
#include "StaticMesh.h"
#include "Material.h"
#include "directxtk/SimpleMath.h"

using namespace SimpleMath;

namespace MyEngine
{
	struct RigidBone
	{
		int parent = -1;
		Matrix local; //상수 값 - 로드할 때 한번만 연산
		Matrix model; //프레임당 한번만 연산
	};

	class RigidMesh : public StaticMesh
	{
	private:
		std::vector<UINT> m_boneIdx;
	public:
		void SetBoneIdx(std::vector<UINT>&& boneIdx) { m_boneIdx = std::move(boneIdx); }
		inline const std::vector<UINT>& GetBoneIndices() const { return m_boneIdx; }
	};

	class RigidMeshRenderer
	{
	private:
		RigidMesh m_rigidMesh;
		std::vector<RigidBone> m_bones;
		std::vector<Material> m_materials;
		// Bind -> 애니메이션 상수버퍼를 올려줌 ( matrix[128],matIdx )
		ComPtr<ID3D11Buffer> m_boneCB;
	public:
		inline void SetMesh(RigidMesh&& mesh) { m_rigidMesh = std::move(mesh); }
		inline void SetBone(std::vector<RigidBone>&& bone) { m_bones = std::move(bone); }
		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		void Draw(ID3D11DeviceContext* context);
	};
}