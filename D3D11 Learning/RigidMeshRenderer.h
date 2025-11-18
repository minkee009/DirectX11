#pragma once
#include <vector>
#include <map>
#include "IRenderable.h"
#include "StaticMesh.h"
#include "Material.h"
#include "directxtk/SimpleMath.h"
#include "Animation.hpp"
#include "MeshRenderer.h"

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
	};

	struct RigidBonePose
	{
		Matrix local; 
		Matrix model;
	};

	class RigidMesh : public StaticMesh
	{
	private:
		std::vector<RigidBone> m_bones;
		std::vector<RigidBonePose> m_initBonePoses;
		std::vector<UINT> m_boneIndices;
	public:
		void SetBones(std::vector<RigidBone>&& bones) { m_bones = std::move(bones); }
		void SetInitBonePoses(std::vector<RigidBonePose>&& bonePoses) { m_initBonePoses = std::move(bonePoses); }

		inline std::vector<RigidBone>& GetBones() { return m_bones; }
		inline std::vector<RigidBonePose>& GetInitBonePoses() { return m_initBonePoses; }

		void SetBoneIndices(std::vector<UINT>&& indices) { m_boneIndices = std::move(indices); }
		inline std::vector<UINT>& GetBoneIndices() { return m_boneIndices; }
	};

	class RigidMeshRenderer : public MeshRenderer
	{
	private:
		std::shared_ptr<RigidMesh> m_pRigidMesh;
		std::vector<RigidBonePose> m_bonePoses;
		ComPtr<ID3D11Buffer> m_boneMatrixCB;
		ComPtr<ID3D11Buffer> m_boneMatrixIdxCB;
		std::unique_ptr<RigidBoneMatCB> m_pBoneMatrixData;
	public:
		RigidMeshRenderer();
		inline void SetMesh(std::shared_ptr<RigidMesh> mesh) { m_pRigidMesh = mesh; }
		inline void SetBonePoses(std::vector<RigidBonePose>&& poses) { m_bonePoses = std::move(poses); }

		void CreateBoneMatrixBuffer(ID3D11DeviceContext* context);

		void Draw(ID3D11DeviceContext* context) override;
		const BoundingBox& GetBBox() override { return m_pRigidMesh->GetBBox(); }
		void MatrixUpdate();

		// ====== 局聪皋捞记 贸府 ====== //
	private:
		bool m_playing = true;
		std::shared_ptr<std::vector<std::unordered_map<UINT, AnimationClip>>> m_pBoneAnimations;
		double m_time = 0;
		double m_speed = 1.0;
		UINT m_animationIdx = 0;
		
	public:
		void AnimationUpdate();

		inline void SetAnimations(std::shared_ptr<std::vector<std::unordered_map<UINT, AnimationClip>>> animations) { m_pBoneAnimations = animations; }

		void Play();
		void Pause();

		inline void SetTime(double time) { m_time = time; }
		inline void SetSpeed(double speed) { m_speed = speed; }
		inline void SetAnimationIndex(UINT index) { m_animationIdx = index; }

		inline double GetDuration() const { if (m_pBoneAnimations->empty()) return 0.0; else return (*m_pBoneAnimations)[m_animationIdx].begin()->second.duration; }
		inline double GetTime() const { return m_time; }
		inline double GetSpeed() const { return m_speed; }
	};
}