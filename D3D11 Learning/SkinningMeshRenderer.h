#pragma once
#include <vector>
#include <map>
#include "StaticMesh.h"
#include "Material.h"
#include "directxtk/SimpleMath.h"
#include "Animation.hpp"
#include "MeshRenderer.h"

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

		bool hasVertex = false;
		BoundingBox bbox;
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
		void CalcBBox() override;
	};

	class SkinningMeshRenderer : public MeshRenderer
	{
	private:
		SkinningMesh m_skinningMesh;
		ComPtr<ID3D11Buffer> m_boneModelMatrixCB;
		ComPtr<ID3D11Buffer> m_boneOffsetMatrixCB;
		std::unique_ptr<SkinningBoneMatCB> m_pBoneModelMatrixData;
		std::unique_ptr<SkinningBoneMatCB> m_pBoneOffsetMatrixData;
	public:
		SkinningMeshRenderer();
		~SkinningMeshRenderer();
		inline void SetSkinningMesh(SkinningMesh&& mesh) { m_skinningMesh = std::move(mesh); }
		inline SkinningMesh& GetSkinningMesh() { return m_skinningMesh; }

		void CreateBoneMatrixBuffers(ID3D11DeviceContext* context);

		void MatrixUpdate();

		void Draw(ID3D11DeviceContext* context);
		const BoundingBox& GetBBox() override { return m_skinningMesh.GetBBox(); }

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