#pragma once
#include <memory>
#include <SimpleMath.h>
#include "Transform.h"

using namespace DirectX::SimpleMath;

namespace MyEngine
{
	class Camera
	{
	private:
		std::unique_ptr<Transform> m_pTransform;
		float m_fov; // 0 ~ 360 degree
		float m_near; // near plane
		float m_far; // far plane
		float m_aspect; // { width / height } ratio

		bool m_isProjMatrixDirty;
		bool m_isViewMatrixDirty;

		Matrix m_cachedViewMatrix; // view Matrix
		Matrix m_cachedProjMatrix; // projection Matrix

	public:
		Camera();
		~Camera();

		Matrix GetCameraMatrix();  // view * projection Matrix
		Matrix& GetViewMatrix();
		Matrix& GetProjMatrix();

		inline const float& GetFOV() const { return m_fov; }
		inline const float& GetNearPlane() const { return m_near; }
		inline const float& GetFarPlane() const { return m_far; }
		inline const float& GetAspectRatio() const { return m_aspect; }

		inline void SetFOV(float value) { m_fov = value; MarkProjectionMatrixDirty(); }
		inline void SetNearPlane(float value) { m_near = value; MarkProjectionMatrixDirty(); }
		inline void SetFarPlane(float value) { m_far = value; MarkProjectionMatrixDirty(); }
		inline void SetAspectRatio(float value) { m_aspect = value; MarkProjectionMatrixDirty(); }
		inline void SetAspectRatio(float width, float height) { m_aspect = width / height; MarkProjectionMatrixDirty(); }

		inline Transform* GetTransform() { return m_pTransform.get(); }

		void MarkViewMatrixDirty();
		void MarkProjectionMatrixDirty();

		void InputUpdate(float deltaTime); // WASD QE for move, mouse for rotate
	};
}