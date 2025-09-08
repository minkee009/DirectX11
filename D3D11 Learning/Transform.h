#pragma once
#include <DirectXMath.h>
#include <memory>

using namespace DirectX;

namespace MyEngine
{
	class Transform
	{
	private:
		XMFLOAT3 m_localPos;
		XMFLOAT4 m_localRot;
		XMFLOAT3 m_localScale;

		bool m_matrixIsDirty;
		std::weak_ptr<Transform> m_parent;
	public:
		XMMATRIX GetLocalMatrix();
		XMMATRIX GetWorldMatrix();

		XMFLOAT3 GetLocalPosition();
		XMFLOAT4 GetLocalRotation();
		XMFLOAT3 GetLocalScale();

		XMFLOAT3 GetWorldPosition();
		XMFLOAT4 GetWorldRotation();
		XMFLOAT3 GetWorldScale();

		void SetLocalPosition(XMFLOAT3 pos);
		void SetLocalPosition(float x, float y, float z);

		void SetLocalRotation(XMFLOAT4 rot);
		void SetLocalRotation(float w, float x, float y, float z);

		void SetLocalScale(XMFLOAT3 scale);
		void SetLocalScale(float x, float y, float z);

		void SetParent(const std::shared_ptr<Transform>& parent);
	};
}