#pragma once
#include <vector>
#include <map>
#include "StaticMesh.h"
#include "Material.h"
#include "directxtk/SimpleMath.h"
#include "Animation.hpp"

namespace MyEngine
{
	//======= RigidMesh =======//
	struct RigidBoneMatCB
	{
		Matrix matricies[128];
	};
	
	struct RigidBoneMatIdxCB
	{
		UINT index = 0;
		float padding[3] = { 0, };
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
		std::vector<UINT> m_boneIndices;
	public:
		void SetBones(std::vector<RigidBone>&& bones) { m_bones = std::move(bones); }
		inline std::vector<RigidBone>& GetBones() { return m_bones; }
		void SetBoneIndices(std::vector<UINT>&& indices) { m_boneIndices = std::move(indices); }
		inline std::vector<UINT>& GetBoneIndices() { return m_boneIndices; }
	};

	class RigidMeshRenderer
	{
	private:
		RigidMesh m_rigidMesh;
		std::vector<Material> m_materials;
		ComPtr<ID3D11Buffer> m_boneMatrixCB;
		ComPtr<ID3D11Buffer> m_boneMatrixIdxCB;
		std::unique_ptr<RigidBoneMatCB> m_pBoneMatrixData;
	public:
		RigidMeshRenderer();
		inline void SetMesh(RigidMesh&& mesh) { m_rigidMesh = std::move(mesh); }
		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		void Draw(ID3D11DeviceContext* context, bool bindMesh = true, bool bindMaterial = true);
		void MatrixUpdate();

		// ====== 애니메이션 처리 ====== //
	private:
		bool m_playing = true;
		std::vector<std::unordered_map<UINT, AnimationClip>> m_boneAnimations;
		double m_time = 0;
		double m_speed = 1.0;
		UINT m_animationIdx = 0;
		
	public:
		void AnimationUpdate();

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