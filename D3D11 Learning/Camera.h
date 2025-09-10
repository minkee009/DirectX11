#pragma once
#include <memory>
#include <d3d11.h>
#include <directxmath.h>
#include "Transform.h"

using namespace DirectX;

namespace MyEngine
{
	class Camera
	{
	private:
		std::unique_ptr<Transform> m_transform;
		float m_fov; // 0 ~ 360 degree
		float m_near; // near plane
		float m_far; // far plane
		float m_aspect; // { width / height } ratio

		XMMATRIX m_cachedInverseMat;

	public:
		XMMATRIX& GetCameraMatrix();
	};
}