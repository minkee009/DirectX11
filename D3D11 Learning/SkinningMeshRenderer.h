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
	};

	struct SkinningBonePose
	{
		Matrix local; //로컬행렬 프레임당 한번만 연산
		Matrix model; //프레임당 한번만 연산
	};

	class SkinningMesh : public StaticMesh
	{
	private:
		std::vector<SkinningBone> m_bones;
		std::unique_ptr<SkinningBoneMatCB> m_pBoneOffsetMatrixData;
		ComPtr<ID3D11Buffer> m_pBoneOffsetMatrixCB;
	public:
		SkinningMesh();
		~SkinningMesh();

		void SetBones(std::vector<SkinningBone>&& bones) { m_bones = std::move(bones); }
		inline std::vector<SkinningBone>& GetBones() { return m_bones; }

		inline void CreateBoneOffsetMatrixBuffer(ID3D11DeviceContext* context);
		inline ID3D11Buffer* GetBoneOffsetMatirxBuffer() { return m_pBoneOffsetMatrixCB.Get(); }
		inline ID3D11Buffer** GetBoneOffsetMatirxBufferAddress() { return m_pBoneOffsetMatrixCB.GetAddressOf(); }
	};

	class SkinningMeshRenderer : public MeshRenderer
	{
	private:
		std::shared_ptr<SkinningMesh> m_pSkinningMesh;
		std::unique_ptr<SkinningBoneMatCB> m_pBoneModelMatrixData;
		ComPtr<ID3D11Buffer> m_pBoneModelMatrixCB;
		std::vector<SkinningBonePose> m_bonePoses;
		
		bool m_isBBoxCalculated = false;
		BoundingBox m_bbox;
	public:
		SkinningMeshRenderer();
		~SkinningMeshRenderer();
		inline void SetSkinningMesh(std::shared_ptr<SkinningMesh> mesh) { m_pSkinningMesh = mesh; }
		inline void SetBonePoses(std::vector<SkinningBonePose>&& poses) { m_bonePoses = std::move(poses); }

		inline SkinningMesh& GetSkinningMesh() { return *m_pSkinningMesh; }
		inline const std::vector<SkinningBonePose>& GetBonePoses() const { return m_bonePoses; }

		void CreateBoneModelMatrixBuffer(ID3D11DeviceContext* context);

		void MatrixUpdate();

		void Draw(ID3D11DeviceContext* context);
		void CalcBBox();

		const BoundingBox& GetBBox() override { if (!m_isBBoxCalculated) { CalcBBox(); m_isBBoxCalculated = true; } return m_bbox; }

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