#include "Camera.h"

MyEngine::Camera::Camera()
{
    m_pTransform = std::make_unique<Transform>();
    m_pTransform->SetOnMatrixUpdated([this] {
        this->MarkCameraMatrixDirty();
        });

    m_fov = 75;
    m_near = 0.001f;
    m_far = 1000.0f;
    m_aspect = 4 / 3;

    m_isCamMatrixDirty = true;
}

MyEngine::Camera::~Camera()
{

}

Matrix& MyEngine::Camera::GetCameraMatrix()
{
    if (m_isCamMatrixDirty)
    {
        //view * projection 매트릭스 생성

        m_isCamMatrixDirty = false;
    }

    return m_cachedCameraMatrix;
}

void MyEngine::Camera::MarkCameraMatrixDirty()
{
    m_isCamMatrixDirty = true;
}
