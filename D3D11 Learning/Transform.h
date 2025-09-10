#pragma once
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

namespace MyEngine
{
	class Transform
	{
	private:
		XMFLOAT3 m_localPos;
		XMFLOAT4 m_localRot;
		XMFLOAT3 m_localScale;

		mutable bool m_isMatrixDirty;

		Transform* m_parent;
		std::vector<Transform*> m_childs;

		mutable XMMATRIX m_cachedMatrix;

		void AddChild(Transform* child);
		void RemoveChild(Transform* child);
		void MarkDirty();
	public:
		XMMATRIX& GetLocalMatrix() const;
		XMMATRIX& GetWorldMatrix() const;

		XMFLOAT3& GetLocalPosition() const;
		XMFLOAT4& GetLocalRotation() const;
		XMFLOAT3& GetLocalScale() const;

		XMFLOAT3& GetWorldPosition() const;
		XMFLOAT4& GetWorldRotation() const;
		XMFLOAT3& GetWorldScale() const;

		void SetLocalPosition(XMFLOAT3 pos);
		void SetLocalPosition(float x, float y, float z);

		void SetLocalRotation(XMFLOAT4 rot);
		void SetLocalRotation(float w, float x, float y, float z);

		void SetLocalEulerRotation(XMFLOAT3 rot); // use degree (0 ~ 360)
		void SetLocalEulerRotation(float x, float y, float z); // use degree (0 ~ 360)

		void SetLocalScale(XMFLOAT3 scale);
		void SetLocalScale(float x, float y, float z);

		inline void SetParent(Transform* parent) { m_parent = parent; }
		inline Transform* GetParent() const { return m_parent; }
	};
}