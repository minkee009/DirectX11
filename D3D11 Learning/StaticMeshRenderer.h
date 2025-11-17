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
		std::shared_ptr<StaticMesh> m_pStaticMesh;
	public:
		inline const StaticMesh& GetMesh() const { return *m_pStaticMesh; }
		inline void SetMesh(std::shared_ptr<StaticMesh> mesh) { m_pStaticMesh = mesh; }

		void Draw(ID3D11DeviceContext* context) override;
		const BoundingBox& GetBBox() override { return m_pStaticMesh->GetBBox(); }
	};
}

