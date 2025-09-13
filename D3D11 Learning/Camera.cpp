#include "Camera.h"
#include <Keyboard.h>
#include <Mouse.h>

MyEngine::Camera::Camera()
{
    m_pTransform = std::make_unique<Transform>();
    m_pTransform->SetOnMatrixUpdated([this] {
        this->MarkViewMatrixDirty();
        });

    m_fov = 75;
    m_near = 0.001f;
    m_far = 1000.0f;
    m_aspect = 4 / 3;

    MarkViewMatrixDirty();
	MarkProjectionMatrixDirty();
}

MyEngine::Camera::~Camera()
{

}

Matrix MyEngine::Camera::GetCameraMatrix()
{
	return GetViewMatrix() * GetProjMatrix();
}

Matrix& MyEngine::Camera::GetViewMatrix()
{
    if (m_isViewMatrixDirty)
    {
		m_cachedViewMatrix = m_pTransform->GetWorldMatrix().Invert();
        m_isViewMatrixDirty = false;
    }

    return m_cachedViewMatrix;
}

Matrix& MyEngine::Camera::GetProjMatrix()
{
    if (m_isProjMatrixDirty)
    {
		m_cachedProjMatrix = Matrix::CreatePerspectiveFieldOfView(m_fov, m_aspect, m_near, m_far);
        m_isProjMatrixDirty = false;
    }

    return m_cachedProjMatrix;
}

void MyEngine::Camera::MarkViewMatrixDirty()
{
	m_isViewMatrixDirty = true;
}

void MyEngine::Camera::MarkProjectionMatrixDirty()
{
    m_isProjMatrixDirty = true;
}

void MyEngine::Camera::InputUpdate(float deltaTime)
{
    auto kb = DirectX::Keyboard::Get().GetState();
    auto mouse = DirectX::Mouse::Get().GetState();
    const float moveSpeed = 5.0f; // units per second
    const float rotSpeed = 0.1f; // radians per pixel
   
    if (mouse.leftButton)
    {
        // Move
        if (kb.W) {
            m_pTransform->SetLocalPosition(m_pTransform->GetLocalPosition() + m_pTransform->GetWorldMatrix().Forward() * moveSpeed * deltaTime);
        }
        if (kb.S) {
            m_pTransform->SetLocalPosition(m_pTransform->GetLocalPosition() - m_pTransform->GetWorldMatrix().Forward() * moveSpeed * deltaTime);
        }
        if (kb.A) {
            m_pTransform->SetLocalPosition(m_pTransform->GetLocalPosition() - m_pTransform->GetWorldMatrix().Right() * moveSpeed * deltaTime);
        }
        if (kb.D) {
            m_pTransform->SetLocalPosition(m_pTransform->GetLocalPosition() + m_pTransform->GetWorldMatrix().Right() * moveSpeed * deltaTime);
        }
        if (kb.Q) {
            m_pTransform->SetLocalPosition(m_pTransform->GetLocalPosition() - m_pTransform->GetWorldMatrix().Up() * moveSpeed * deltaTime);
        }
        if (kb.E) {
            m_pTransform->SetLocalPosition(m_pTransform->GetLocalPosition() + m_pTransform->GetWorldMatrix().Up() * moveSpeed * deltaTime);
        }

        // Rotate
        auto rot = m_pTransform->GetLocalRotation().ToEuler();
        rot.y += mouse.x * rotSpeed;
        rot.x += mouse.y * rotSpeed;
        // Limit pitch to avoid gimbal lock
        if (rot.x > 89.0f) rot.x = 89.0f;
        if (rot.x < -89.0f) rot.x = -89.0f;
        m_pTransform->SetLocalEulerRotation(rot);
	}
}


