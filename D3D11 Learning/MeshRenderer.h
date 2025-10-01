#pragma once
#include <unordered_map>
#include <string>

#include "Mesh.h"
#include "Material.h"

namespace MyEngine
{
	class MeshRenderer
	{
	private:
		Mesh* m_pMesh = nullptr; //소유권 없음(참조만)
		std::unordered_map<std::wstring, Material*> m_materials; //소유권 없음(참조만)

		//=== 임시 인풋 레이아웃 코드 ===//
		static constexpr D3D11_INPUT_ELEMENT_DESC s_inputDesc_Pos[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		static constexpr D3D11_INPUT_ELEMENT_DESC s_inputDesc_PosNormUV[3] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		//=============================//

		const D3D11_INPUT_ELEMENT_DESC* GetInputDesc(const VertexType& type) const;

		ComPtr<ID3D11InputLayout> m_inputLayout = nullptr;  //추후에 인풋 레이아웃 매니저에서 관리하는걸로 변경

	public:
		MeshRenderer() {}
		~MeshRenderer() {}
		inline void SetMesh(Mesh* mesh) { m_pMesh = mesh; }	

		void Draw(ID3D11DeviceContext* ctx);
	};
}