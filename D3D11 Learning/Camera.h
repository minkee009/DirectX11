#pragma once
#include <memory>
#include <d3d11.h>
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

		bool m_isCamMatrixDirty;

		Matrix m_cachedCameraMatrix; // view * projection Matrix

	public:
		Camera();
		~Camera();

		Matrix& GetCameraMatrix();  // view * projection Matrix
		void MarkCameraMatrixDirty();
	};
}