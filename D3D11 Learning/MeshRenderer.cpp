#include "MeshRenderer.h"

const D3D11_INPUT_ELEMENT_DESC* MyEngine::MeshRenderer::GetInputDesc(const VertexType& type) const
{
	switch (type)
	{
	case VertexType::Pos:
		return s_inputDesc_Pos;
	case VertexType::PosNormUV:
		return s_inputDesc_PosNormUV;
	}

	return nullptr;
}

void MyEngine::MeshRenderer::Draw(ID3D11DeviceContext* ctx)
{
	if (!m_pMesh)
		return;

	if (!m_inputLayout)
	{
		//임시 인풋 레이아웃 생성
		const D3D11_INPUT_ELEMENT_DESC* layout = GetInputDesc(m_pMesh->GetVertexType());
		UINT numElements = 0;
		switch (m_pMesh->GetVertexType())
		{
		case VertexType::Pos:
			numElements = ARRAYSIZE(s_inputDesc_Pos);
			break;
		case VertexType::PosNormUV:
			numElements = ARRAYSIZE(s_inputDesc_PosNormUV);
			break;
		}
		if (m_materials.empty())
		{
			//없는 경우 기본 머터리얼로 인풋 레이아웃 생성
			ID3D11Device* device = nullptr;
			ctx->GetDevice(&device);
			HRESULT hr = device->CreateInputLayout(layout, numElements,
				Material::GetDefaultVSBlob()->GetBufferPointer(), Material::GetDefaultVSBlob()->GetBufferSize(),
				m_inputLayout.GetAddressOf());
			if (FAILED(hr))
				return;
		}
		else
		{
			auto it = m_materials.begin();
			ID3D11Device* device = nullptr;
			ctx->GetDevice(&device);
			HRESULT hr = device->CreateInputLayout(layout, numElements,
				it->second->GetVSBlob()->GetBufferPointer(), it->second->GetVSBlob()->GetBufferSize(),
				m_inputLayout.GetAddressOf());

			if (FAILED(hr))
				return;
		}
	}

	auto& subMeshes = m_pMesh->GetSubMeshes();
	for (size_t i = 0; i < subMeshes.size(); i++)
	{
		auto& subMesh = subMeshes[i];

		//머터리얼 바인딩
		auto it = m_materials.find(subMesh.materialName);
		if (it != m_materials.end())
		{
			it->second->Bind(ctx);
		}
		else
		{
			//없는 경우 보라색 머터리얼 바인딩
			ctx->VSSetShader(Material::GetDefaultVertexShader(), nullptr, 0);
			ctx->PSSetShader(Material::GetDefaultPixelShader(), nullptr, 0);
		}

		//인풋 레이아웃 설정
		ctx->IASetInputLayout(it != m_materials.end() ? m_inputLayout.Get() : nullptr);

		//버퍼 설정
		constexpr UINT stride = sizeof(MeshVertex);
		constexpr UINT offset = 0;
		ctx->IASetVertexBuffers(0, 1, subMesh.pVertexBuffer.GetAddressOf(), &stride, &offset);
		ctx->IASetIndexBuffer(subMesh.pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//드로우 콜
		ctx->DrawIndexed(static_cast<UINT>(subMesh.indices.size()), 0, 0);
	}
}
