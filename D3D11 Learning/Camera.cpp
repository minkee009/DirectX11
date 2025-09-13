#include "Camera.h"

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


