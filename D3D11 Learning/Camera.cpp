#define NOMINMAX
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

const Matrix MyEngine::Camera::GetCameraMatrix()
{
	return GetViewMatrix() * GetProjMatrix();
}

const Matrix& MyEngine::Camera::GetViewMatrix()
{
    if (m_isViewMatrixDirty)
    {
		m_cachedViewMatrix = m_pTransform->GetWorldMatrix().Invert();
        m_isViewMatrixDirty = false;
    }

    return m_cachedViewMatrix;
}

const Matrix& MyEngine::Camera::GetProjMatrix()
{
    if (m_isProjMatrixDirty)
    {
        m_cachedProjMatrix = Matrix::CreatePerspectiveFieldOfView(DirectX::XMConvertToRadians(m_fov), m_aspect, m_near, m_far);
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

    // 트래커 업데이트
    m_mouseTracker.Update(mouse);
    m_keyboardTracker.Update(kb);

    const float moveSpeed = 5.0f;
    const float rotSpeed = 0.1f;

    if (mouse.rightButton)
    {
        // 마우스 회전 처리
        if (!m_firstMouseUpdate)
        {
            int deltaX = mouse.x - m_lastMouseX;
            int deltaY = mouse.y - m_lastMouseY;

            if (deltaX != 0 || deltaY != 0)
            {
                // 현재 회전값을 도 단위로 가져오기
                auto rot = m_pTransform->GetLocalRotation().ToEuler();
                float pitch = DirectX::XMConvertToDegrees(rot.x);
                float yaw = DirectX::XMConvertToDegrees(rot.y);
                float roll = DirectX::XMConvertToDegrees(rot.z);

                // 마우스 델타로 회전 적용
                yaw -= deltaX * rotSpeed;
                pitch -= deltaY * rotSpeed; // Y는 반대 방향

                // Pitch 제한 (짐벌 락 방지)
                pitch = std::max(-89.0f, std::min(89.0f, pitch));

                // 새 회전값 설정 (도 단위)
                m_pTransform->SetLocalEulerRotation(Vector3(pitch, yaw, roll));
            }
        }
        else
        {
            m_firstMouseUpdate = false;
        }

        // 이동 처리 (더 효율적으로)
        Vector3 movement = Vector3::Zero;
        Matrix worldMat = m_pTransform->GetWorldMatrix();

        if (kb.W) movement += worldMat.Forward();
        if (kb.S) movement -= worldMat.Forward();
        if (kb.A) movement -= worldMat.Right();
        if (kb.D) movement += worldMat.Right();
        if (kb.Q) movement -= worldMat.Up();
        if (kb.E) movement += worldMat.Up();

        // 정규화 후 이동 적용
        if (movement.LengthSquared() > 0.0f)
        {
            movement.Normalize();
            Vector3 currentPos = m_pTransform->GetLocalPosition();
            m_pTransform->SetLocalPosition(currentPos + movement * moveSpeed * deltaTime);
        }
    }
    else
    {
        // 우클릭이 해제되면 마우스 업데이트 초기화
        if (m_mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::RELEASED)
        {
            m_firstMouseUpdate = true;
        }
    }

    // 마우스 위치 업데이트
    m_lastMouseX = mouse.x;
    m_lastMouseY = mouse.y;
}


