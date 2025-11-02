#pragma once
#include <vector>
#include <map>
#include "StaticMesh.h"
#include "Material.h"
#include "directxtk/SimpleMath.h"
#include "Animation.hpp"
#include "AABB.h"

namespace MyEngine
{
	//======= Skinning Mesh =======//
	struct SkinningBoneMatCB
	{
		Matrix matricies[128];
	};

	struct SkinningBone
	{
		int index = -1;
		int parentIndex = -1;


		AABB boundBox;
		Matrix offset;  //바인드 역행렬 <- 상수 값
		Matrix local; //로컬행렬 프레임당 한번만 연산
		Matrix model; //프레임당 한번만 연산
	};

	class SkinningMesh : public StaticMesh
	{
	private:
		std::vector<SkinningBone> m_bones;
	public:
		void SetBones(std::vector<SkinningBone>&& bones) { m_bones = std::move(bones); }
		inline std::vector<SkinningBone>& GetBones() { return m_bones; }
	};

	class SkinningMeshRenderer
	{
	private:
		SkinningMesh m_skinningMesh;
		std::vector<Material> m_materials;
		ComPtr<ID3D11Buffer> m_boneModelMatrixCB;
		ComPtr<ID3D11Buffer> m_boneOffsetMatrixCB;
		std::unique_ptr<SkinningBoneMatCB> m_boneModelMatrixData;
		std::unique_ptr<SkinningBoneMatCB> m_boneOffsetMatrixData;
	public:
		inline void SetMesh(SkinningMesh&& mesh) { m_skinningMesh = std::move(mesh); }
		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		void Draw(ID3D11DeviceContext* context, bool bindMesh = true, bool bindMaterial = true, bool updateMatrix = true);

		// ====== 애니메이션 처리 ====== //
	private:
		bool m_playing = true;
		std::vector<std::unordered_map<UINT, AnimationClip>> m_boneAnimations;
		double m_time = 0;
		double m_speed = 1.0;
		UINT m_animationIdx = 0;
	public:
		void MatrixUpdate();
		inline void SetAnimations(std::vector<std::unordered_map<UINT, AnimationClip>>&& animations) { m_boneAnimations = std::move(animations); }

		void Play();
		void Pause();

		inline void SetTime(double time) { m_time = time; }
		inline void SetSpeed(double speed) { m_speed = speed; }
		inline void SetAnimationIndex(UINT index) { m_animationIdx = index; }

		inline double GetDuration() const { if (m_boneAnimations.empty()) return 0.0; else return m_boneAnimations[m_animationIdx].begin()->second.duration; }
		inline double GetTime() const { return m_time; }
		inline double GetSpeed() const { return m_speed; }
	};
}