#include "Transform.h"

void MyEngine::Transform::AddChild(Transform* child)
{
	if (child == nullptr || child == this) return; // 자기 자신이나 nullptr은 추가하지 않음
	// 이미 자식으로 등록되어 있는지 확인
	for (const auto& existingChild : m_childs)
	{
		if (existingChild == child) return; // 이미 자식으로 등록되어 있으면 추가하지 않음
	}
	m_childs.push_back(child);
	child->m_parent = this; // 부모 설정
}

void MyEngine::Transform::RemoveChild(Transform* child)
{
	if (child == nullptr) return; // nullptr은 제거하지 않음
	auto it = std::find(m_childs.begin(), m_childs.end(), child);
	if (it != m_childs.end())
	{
		m_childs.erase(it); // 자식 목록에서 제거
		child->m_parent = nullptr; // 부모 설정 해제
	}
}

void MyEngine::Transform::MarkDirty()
{
	m_isMatrixDirty = true; // 행렬이 더러워졌음을 표시
	for (Transform* child : m_childs)
	{
		child->MarkDirty(); // 자식들도 더러워졌다고 표시
	}
}

MyEngine::Transform::Transform()
	: m_localPosition(0.0f, 0.0f, 0.0f)
	, m_localRotation(0.0f, 0.0f, 0.0f, 1.0f)
	, m_localScale(1.0f, 1.0f, 1.0f)
	, m_parent(nullptr)
	, m_isMatrixDirty(true)
	, m_cachedMatrix(Matrix::Identity)
{

}

MyEngine::Transform::~Transform()
{
	if (!m_childs.empty())
	{
		for (auto child : m_childs)
		{
			child->SetParent(nullptr);
		}
	}

	SetParent(nullptr);
}

Matrix& MyEngine::Transform::GetLocalMatrix() const
{
	if (m_isMatrixDirty)
	{
		m_cachedMatrix = 
			Matrix::CreateScale(m_localScale) *
			Matrix::CreateFromQuaternion(m_localRotation) *
			Matrix::CreateTranslation(m_localPosition);

		m_isMatrixDirty = false;
	}

	return m_cachedMatrix;
}

Matrix MyEngine::Transform::GetWorldMatrix() const
{
	if (m_parent)
		return m_parent->GetWorldMatrix() * GetLocalMatrix();
	else
		return GetLocalMatrix();
}

const Vector3& MyEngine::Transform::GetLocalPosition() const
{
	return m_localPosition;
}

const Quaternion& MyEngine::Transform::GetLocalRotation() const
{
	return m_localRotation;
}

const Vector3& MyEngine::Transform::GetLocalScale() const
{
	return m_localScale;
}

const Vector3 MyEngine::Transform::GetWorldPosition() const
{
	if (m_parent)
	{
		// 부모의 월드 행렬을 적용
		return Vector3::Transform(m_localPosition, m_parent->GetWorldMatrix());
	}

	return m_localPosition;
}

const Quaternion MyEngine::Transform::GetWorldRotation() const
{
	if (m_parent)
	{
		return m_parent->GetWorldRotation() * m_localRotation;
	}
	return m_localRotation;
}

const Vector3 MyEngine::Transform::GetWorldScale() const
{
	if (m_parent)
	{
		Vector3 parentScale = m_parent->GetWorldScale();
		return Vector3 {
			m_localScale.x * parentScale.x,
			m_localScale.y * parentScale.y,
			m_localScale.z * parentScale.z
		};
	}
	return m_localScale;
}

void MyEngine::Transform::SetWorldPosition(Vector3 pos)
{
	if (m_parent)
	{
		//부모의 역행렬로 돌아간뒤 pos값을 적용해야 함
		auto parentInvWorldMat = m_parent->GetWorldMatrix().Invert();
		m_localPosition = Vector3::Transform(pos, parentInvWorldMat);  // pos를 역변환
	}
	else
	{
		m_localPosition = pos;
	}

	MarkDirty();
	if (m_onMatrixUpdated) 
		m_onMatrixUpdated();
}

void MyEngine::Transform::SetWorldRotation(Quaternion rot)
{
	if (m_parent)
	{
		Quaternion parentInvQuater = Quaternion::Identity;
		m_parent->GetWorldRotation().Inverse(parentInvQuater);

		m_localRotation = parentInvQuater * rot;
	}
	else
	{
		m_localRotation = rot;
	}

	MarkDirty();
	if (m_onMatrixUpdated)
		m_onMatrixUpdated();
}

void MyEngine::Transform::SetWorldEulerRotation(Vector3 rot) // use degree (0 ~ 360)
{
	// 오일러 각(도 단위)을 라디안 단위로 변환
	auto radRotX = DirectX::XMConvertToRadians(rot.x);
	auto radRotY = DirectX::XMConvertToRadians(rot.y);
	auto radRotZ = DirectX::XMConvertToRadians(rot.z);

	// 라디안 오일러 각으로 쿼터니언 생성
	Quaternion worldRot = Quaternion::CreateFromYawPitchRoll(radRotY ,radRotX, radRotZ);

	SetWorldRotation(worldRot);
}

void MyEngine::Transform::SetWorldScale(Vector3 scale)
{
	if (m_parent)
	{
		Vector3 parentScale = m_parent->GetWorldScale();
		m_localScale = Vector3(
			scale.x / parentScale.x,
			scale.y / parentScale.y,
			scale.z / parentScale.z
		);
	}
	else
	{
		m_localScale = scale;
	}

	MarkDirty();
	if (m_onMatrixUpdated)
		m_onMatrixUpdated();
}

void MyEngine::Transform::SetLocalPosition(Vector3 pos)
{
	m_localPosition = pos;
	MarkDirty();
	if (m_onMatrixUpdated)
		m_onMatrixUpdated();
}

void MyEngine::Transform::SetLocalRotation(Quaternion rot)
{
	m_localRotation = rot;
	MarkDirty();
	if (m_onMatrixUpdated)
		m_onMatrixUpdated();
}

void MyEngine::Transform::SetLocalEulerRotation(Vector3 rot)
{
	// 오일러 각(도 단위)을 라디안 단위로 변환
	auto radRotX = DirectX::XMConvertToRadians(rot.x);
	auto radRotY = DirectX::XMConvertToRadians(rot.y);
	auto radRotZ = DirectX::XMConvertToRadians(rot.z);

	// 라디안 오일러 각으로 쿼터니언 생성
	Quaternion worldRot = Quaternion::CreateFromYawPitchRoll(radRotY, radRotX, radRotZ);

	SetLocalRotation(worldRot);
}

void MyEngine::Transform::SetLocalScale(Vector3 scale)
{
	m_localScale = scale;
	MarkDirty();
	if (m_onMatrixUpdated)
		m_onMatrixUpdated();
}

void MyEngine::Transform::SetParent(Transform* parent, bool worldPositionStays)
{
	// 이미 같은 부모면 return
	if (m_parent == parent) return;

	// 현재 월드 행렬 백업
	Matrix worldMatrixBefore = GetWorldMatrix();

	// 기존 부모에서 제거
	if (m_parent)
		m_parent->RemoveChild(this);

	// 부모 교체
	m_parent = parent;

	if (m_parent)
		m_parent->AddChild(this);

	if (worldPositionStays)
	{
		if (m_parent)
		{
			// 부모 기준 새 로컬 행렬
			Matrix parentWorldInv = m_parent->GetWorldMatrix().Invert();
			Matrix newLocal = worldMatrixBefore * parentWorldInv;

			newLocal.Decompose(m_localScale, m_localRotation, m_localPosition);
		}
		else
		{
			// 부모가 없으면 로컬 = 월드
			worldMatrixBefore.Decompose(m_localScale, m_localRotation, m_localPosition);
		}
	}

	MarkDirty();
	if (m_onMatrixUpdated)
		m_onMatrixUpdated();
}
