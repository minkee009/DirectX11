#pragma once
#include <memory>
#include "StaticMesh.h"
#include "Material.h"
#include "MeshRenderer.h"

namespace MyEngine
{
	class StaticMeshRenderer : public MeshRenderer
	{
	private:
		StaticMesh m_staticMesh;
	public:
		inline const StaticMesh& GetMesh() const { return m_staticMesh; }
		inline void SetMesh(StaticMesh&& mesh) { m_staticMesh = std::move(mesh); }

		void Draw(ID3D11DeviceContext* context) override;
		const BoundingBox& GetBBox() override { return m_staticMesh.GetBBox(); }
	};
}

