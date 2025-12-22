#pragma once
#include <memory>
#include <DirectXTK/SimpleMath.h>
#include <DirectXTK/Keyboard.h>
#include <DirectXTK/Mouse.h>
#include "Transform.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;

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

		mutable bool m_isProjMatrixDirty;
		mutable bool m_isViewMatrixDirty;

		mutable Matrix m_cachedViewMatrix; // view Matrix
		mutable Matrix m_cachedProjMatrix; // projection Matrix

		mutable BoundingFrustum m_cachedProjFrustum;

		DirectX::Mouse::ButtonStateTracker m_mouseTracker;
		DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;
		int m_lastMouseX = 0;
		int m_lastMouseY = 0;
		bool m_firstMouseUpdate = true;

		void UpdateProjMatrix();
		void UpdateProjFrustum();

	public:
		Camera();
		~Camera();

		const Matrix GetCameraMatrix();  // view * projection Matrix
		const Matrix& GetViewMatrix();
		const Matrix& GetProjMatrix();
		const BoundingFrustum& GetProjFrustum();

		inline const float& GetFOV() const { return m_fov; }
		inline const float& GetNearPlane() const { return m_near; }
		inline const float& GetFarPlane() const { return m_far; }
		inline const float& GetAspectRatio() const { return m_aspect; }

		inline void SetFOV(float value) { m_fov = value; MarkProjectionMatrixDirty(); }
		inline void SetNearPlane(float value) { m_near = value; MarkProjectionMatrixDirty(); }
		inline void SetFarPlane(float value) { m_far = m_near + 0.01f > value ? m_near + 0.01f : value; MarkProjectionMatrixDirty(); }
		inline void SetAspectRatio(float value) { m_aspect = value; MarkProjectionMatrixDirty(); }
		inline void SetAspectRatio(float width, float height) { m_aspect = width / height; MarkProjectionMatrixDirty(); }

		inline Transform* GetTransform() { return m_pTransform.get(); }

		void MarkViewMatrixDirty();
		void MarkProjectionMatrixDirty();

		inline const bool IsDirtyMatrix() { return m_isProjMatrixDirty || m_isViewMatrixDirty; }

		void InputUpdate(float deltaTime); // WASD QE for move, mouse for rotate
	};
}