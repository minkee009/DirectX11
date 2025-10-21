#include "RigidMeshRenderer.h"

void MyEngine::RigidMeshRenderer::Draw(ID3D11DeviceContext* context)
{
	if (!m_boneCB)
	{
		//상수버퍼 만들어주기
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(MaterialCB);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;

		ID3D11Device* pDevice;
		context->GetDevice(&pDevice);

		HRESULT hr = pDevice->CreateBuffer(&cbDesc, nullptr, m_boneCB.GetAddressOf());
		if (FAILED(hr))
			return;
	}

	//상수버퍼에 올릴 model버퍼 연산 및 집어넣기
	std::vector<Matrix> modelMats;

	for (UINT i = 0; i < m_bones.size(); i++)
	{
		auto& bone = m_bones[i];
		
		if (bone.parent != -1)
		{
			auto& boneParent = m_bones[bone.parent];
			bone.model = boneParent.model * bone.local;
		}
		else
		{
			bone.model = bone.local;
		}

		modelMats.push_back(bone.model);
	}
}
