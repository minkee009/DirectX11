#pragma once
#include <SimpleMath.h>
#include <vector>

using namespace DirectX::SimpleMath;

namespace MyEngine
{
	class Transform
	{
	private:
		Vector3 m_localPos;
		Quaternion m_localRot;
		Vector3 m_localScale;

		mutable bool m_isMatrixDirty;

		Transform* m_parent;
		std::vector<Transform*> m_childs;

		mutable Matrix m_cachedMatrix;

		void AddChild(Transform* child);
		void RemoveChild(Transform* child);
		void MarkDirty();
	public:
		Matrix& GetLocalMatrix() const;
		Matrix& GetWorldMatrix() const;

		Vector3& GetLocalPosition() const;
		Quaternion& GetLocalRotation() const;
		Vector3& GetLocalScale() const;

		Vector3& GetWorldPosition() const;
		Quaternion& GetWorldRotation() const;
		Vector3& GetWorldScale() const;

		void SetWorldPosition(Vector3 pos);
		void SetWorldPosition(float x, float y, float z);
				
		void SetWorldRotation(Quaternion rot);
		void SetWorldRotation(float x, float y, float z, float w);
				
		void SetWorldEulerRotation(Vector3 rot); // use degree (0 ~ 360)
		void SetWorldEulerRotation(float x, float y, float z); // use degree (0 ~ 360)
				
		void SetWorldScale(Vector3 scale);
		void SetWorldScale(float x, float y, float z);


		void SetLocalPosition(Vector3 pos);
		void SetLocalPosition(float x, float y, float z);

		void SetLocalRotation(Quaternion rot);
		void SetLocalRotation(float x, float y, float z, float w);

		void SetLocalEulerRotation(Vector3 rot); // use degree (0 ~ 360)
		void SetLocalEulerRotation(float x, float y, float z); // use degree (0 ~ 360)

		void SetLocalScale(Vector3 scale);
		void SetLocalScale(float x, float y, float z);

		inline void SetParent(Transform* parent) { m_parent = parent; }
		inline Transform* GetParent() const { return m_parent; }
	};
}