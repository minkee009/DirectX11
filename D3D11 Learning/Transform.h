#pragma once
#include <DirectXTK/SimpleMath.h>
#include <vector>

using namespace DirectX::SimpleMath;

namespace MyEngine
{
	class Transform
	{
	private:
		Vector3 m_localPosition;
		Quaternion m_localRotation;
		Vector3 m_localScale;

		mutable bool m_isMatrixDirty;

		Transform* m_parent;
		std::vector<Transform*> m_childs;

		std::function<void()> m_onMatrixUpdated;

		mutable Matrix m_cachedMatrix;

		void AddChild(Transform* child);
		void RemoveChild(Transform* child);
		void MarkDirty();
	public:
		Transform();
		~Transform();

		Matrix& GetLocalMatrix() const;
		Matrix GetWorldMatrix() const;

		const Vector3& GetLocalPosition() const;
		const Quaternion& GetLocalRotation() const;
		const Vector3 GetLocalEulerRotation() const; // return degree (0 ~ 360)
		const Vector3& GetLocalScale() const;

		const Vector3 GetWorldPosition() const;
		const Quaternion GetWorldRotation() const;
		const Vector3 GetWorldEulerRotation() const; // return degree (0 ~ 360)
		const Vector3 GetWorldScale() const;

		void SetWorldPosition(Vector3 pos);
		inline void SetWorldPosition(float x, float y, float z) { SetWorldPosition({ x,y,z }); }
	
		void SetWorldRotation(Quaternion rot);
		inline void SetWorldRotation(float x, float y, float z, float w) { SetWorldRotation({ x,y,z,w }); }
				
		void SetWorldEulerRotation(Vector3 rot); // use degree (0 ~ 360)
		inline void SetWorldEulerRotation(float x, float y, float z) { SetWorldEulerRotation({ x,y,z }); } // use degree (0 ~ 360)
				
		void SetWorldScale(Vector3 scale);
		inline void SetWorldScale(float x, float y, float z) { SetWorldScale({ x,y,z }); }

		void SetLocalPosition(Vector3 pos);
		inline void SetLocalPosition(float x, float y, float z) { SetLocalPosition({ x,y,z }); }

		void SetLocalRotation(Quaternion rot);
		inline void SetLocalRotation(float x, float y, float z, float w) { SetLocalRotation({ x,y,z,w }); }

		void SetLocalEulerRotation(Vector3 rot); // use degree (0 ~ 360)
		inline void SetLocalEulerRotation(float x, float y, float z) { SetLocalEulerRotation({ x,y,z }); } // use degree (0 ~ 360)

		void SetLocalScale(Vector3 scale);
		inline void SetLocalScale(float x, float y, float z) { SetLocalScale({ x,y,z }); }

		void SetParent(Transform* parent, bool worldPositionStays = true);
		inline Transform* GetParent() const { return m_parent; }

		inline void SetOnMatrixUpdated(std::function<void()> func) { m_onMatrixUpdated = std::move(func); }
	};
}