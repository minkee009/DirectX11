#pragma once
#include <vector>
#include "StaticMesh.h"
#include "Material.h"
#include "directxtk/SimpleMath.h"
#include "Animation.hpp"

namespace MyEngine
{
	//단일 클립 재생기
	//class AnimationController
	//{
	//private:
	//	AnimationClip m_clip;
	//	double m_time;
	//	double m_speed;
	//public:
	//	inline void SetClip(AnimationClip&& clip) { m_clip = std::move(clip); }
	//	void SetSpeed(double speed);
	//	void Play();
	//};


	//======= RigidMesh =======//
	struct BoneMatCB
	{
		Matrix matricies[128];
	};
	
	struct BoneMatIdxCB
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
	public:
		void SetBones(std::vector<RigidBone>&& bones) { m_bones = std::move(bones); }
		inline std::vector<RigidBone>& GetBones() { return m_bones; }
	};

	class RigidMeshRenderer
	{
	private:
		RigidMesh m_rigidMesh;
		std::vector<Material> m_materials;
		ComPtr<ID3D11Buffer> m_boneMatrixCB;
		ComPtr<ID3D11Buffer> m_boneMatrixIdxCB;
	public:
		inline void SetMesh(RigidMesh&& mesh) { m_rigidMesh = std::move(mesh); }
		inline void AddMaterial(Material&& material) { m_materials.emplace_back(material); }
		void Draw(ID3D11DeviceContext* context);

		// ====== 애니메이션 처리 ====== //
	private:
		std::vector<AnimationClip> m_boneAnimClips;
		double m_time;
		double m_speed;
	public:
		inline void SetClip(std::vector<AnimationClip>&& clips) { m_boneAnimClips = std::move(clips); }

		void Play();
		inline void SetSpeed(double speed) { m_speed = speed; }
	};
}