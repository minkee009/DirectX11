#define NOMINMAX
#include "Camera.h"
#include <DirectXTK/Keyboard.h>
#include <DirectXTK/Mouse.h>

namespace CameraMathf
{
    float Lerp(float a, float b, float t)
    {
        return (a + (b - a) * t);
    }
}

MyEngine::Camera::Camera()
{
    m_pTransform = std::make_unique<Transform>();
    m_pTransform->SetOnMatrixUpdated([this] {
        this->MarkViewMatrixDirty();
        });

    m_fov = 75;
    m_near = 0.3f;
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

    static float targetPitch = DirectX::XMConvertToDegrees(m_pTransform->GetLocalRotation().ToEuler().x);
    static float targetYaw = DirectX::XMConvertToDegrees(m_pTransform->GetLocalRotation().ToEuler().y);

    static float pitch = DirectX::XMConvertToDegrees(m_pTransform->GetLocalRotation().ToEuler().x);
    static float yaw = DirectX::XMConvertToDegrees(m_pTransform->GetLocalRotation().ToEuler().y);

    Vector3 targetMovement = Vector3::Zero;
    static Vector3 movement = Vector3::Zero;

    if (mouse.rightButton)
    {
        // 마우스 회전 처리
        if (!m_firstMouseUpdate)
        {
            int deltaX = mouse.x - m_lastMouseX;
            int deltaY = mouse.y - m_lastMouseY;

            if (deltaX != 0 || deltaY != 0)
            {
                // 마우스 델타로 회전 적용
                targetYaw -= deltaX * rotSpeed;
                targetPitch -= deltaY * rotSpeed; // Y는 반대 방향

                // Pitch 제한
                //targetPitch = std::max( -89.0f, std::min(89.0f, targetPitch));
            }
        }
        else
        {
            m_firstMouseUpdate = false;
        }

        // 이동 처리 (더 효율적으로)
        Matrix worldMat = m_pTransform->GetWorldMatrix();

        if (kb.W) targetMovement += worldMat.Forward();
        if (kb.S) targetMovement -= worldMat.Forward();
        if (kb.A) targetMovement -= worldMat.Right();
        if (kb.D) targetMovement += worldMat.Right();
        if (kb.Q) targetMovement -= worldMat.Up();
        if (kb.E) targetMovement += worldMat.Up();
    }
    else
    {
        // 우클릭이 해제되면 마우스 업데이트 초기화
        if (m_mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::RELEASED)
        {
            m_firstMouseUpdate = true;
        }
    }

    pitch = CameraMathf::Lerp(pitch, targetPitch, 12.0f * deltaTime);
    yaw = CameraMathf::Lerp(yaw, targetYaw, 12.0f * deltaTime);

    // 새 회전값 설정 (도 단위)
    m_pTransform->SetLocalEulerRotation(Vector3(pitch, yaw, 0));

    // 정규화 후 이동 적용
    if (targetMovement.LengthSquared() > 0.0f)
    {
        targetMovement.Normalize();
    }

    movement = Vector3::Lerp(movement, targetMovement, 6.0f * deltaTime);

    Vector3 currentPos = m_pTransform->GetLocalPosition();
    m_pTransform->SetLocalPosition(currentPos + movement * moveSpeed * deltaTime);

    // 마우스 위치 업데이트
    m_lastMouseX = mouse.x;
    m_lastMouseY = mouse.y;
}


