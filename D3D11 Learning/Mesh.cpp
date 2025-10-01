#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include <dxgi.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool MyEngine::Mesh::LoadFromFile(ID3D11Device* device, std::wstring path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
		std::wcerr << L"Failed to open file: " << path << std::endl;
		return false;
    }

    std::string line;

	std::vector<XMFLOAT3> temp_vert_pos;
	std::vector<XMFLOAT3> temp_vert_nor;
	std::vector<XMFLOAT2> temp_vert_uv;
	std::vector<UINT> temp_indices;
	bool lastprefixIsUsemtl = false;
    while (std::getline(file, line)) 
    {
        //라인 분석
		std::istringstream iss(line);
		std::string prefix;
		
		iss >> prefix;

        if (prefix == "v") //정점 위치
        {
            XMFLOAT3 pos;
			iss >> pos.x >> pos.y >> pos.z;
			temp_vert_pos.push_back(pos);
		}
		else if (prefix == "vn") //정점 노멀
		{
			XMFLOAT3 nor;
			iss >> nor.x >> nor.y >> nor.z;
			temp_vert_nor.push_back(nor);
        }
		else if (prefix == "vt") //정점 UV
		{
			XMFLOAT2 uv;
			iss >> uv.x >> uv.y;
			uv.y = 1.0f - uv.y;  // Y축 뒤집기 (DirectX 좌표계로 변환)
			temp_vert_uv.push_back(uv);
		}
		else if (prefix == "f") //면
		{
			if (m_subMeshes.empty()) {
				SubMesh first;
				m_subMeshes.emplace_back(first);
			}

			// float/float/float -> 정점임
			// 4묶음도 있음 -> 이러는 경우 (v0, v1, v2) + (v0, v2, v3) 의 형식으로 인덱스를 저장해야 함
			std::string vertexStr;
			std::vector<UINT> faceIndices;
			int vertexCount = 0;
			while (iss >> vertexStr)
			{
				++vertexCount;
				std::istringstream viss(vertexStr);
				std::string vIdxStr, vtIdxStr, vnIdxStr;
				std::getline(viss, vIdxStr, '/');
				std::getline(viss, vtIdxStr, '/');
				std::getline(viss, vnIdxStr, '/');
				UINT vIdx = std::stoi(vIdxStr) - 1; // OBJ 인덱스는 1부터 시작
				UINT vtIdx = vtIdxStr.empty() ? 0 : std::stoi(vtIdxStr) - 1;
				UINT vnIdx = vnIdxStr.empty() ? 0 : std::stoi(vnIdxStr) - 1;
				// 정점 생성 및 임시 저장
				MeshVertex vertex = {};
				vertex.pos = temp_vert_pos[vIdx];
				if (!temp_vert_nor.empty())
					vertex.nor = temp_vert_nor[vnIdx];
				if (!temp_vert_uv.empty())
					vertex.uv = temp_vert_uv[vtIdx];
				// 정점이 이미 존재하는지 확인
				auto it = std::find_if(m_subMeshes.back().vertices.begin(), m_subMeshes.back().vertices.end(),
					[&vertex](const MeshVertex& v) {
						return v.pos.x == vertex.pos.x && v.pos.y == vertex.pos.y && v.pos.z == vertex.pos.z &&
							v.nor.x == vertex.nor.x && v.nor.y == vertex.nor.y && v.nor.z == vertex.nor.z &&
							v.uv.x == vertex.uv.x && v.uv.y == vertex.uv.y;
					});
				if (it != m_subMeshes.back().vertices.end())
				{
					// 이미 존재하면 해당 인덱스 사용
					UINT index = static_cast<UINT>(std::distance(m_subMeshes.back().vertices.begin(), it));
					faceIndices.push_back(index);
				}
				else
				{
					// 새 정점 추가
					m_subMeshes.back().vertices.push_back(vertex);
					UINT newIndex = static_cast<UINT>(m_subMeshes.back().vertices.size() - 1);
					faceIndices.push_back(newIndex);
				}
			}

			// 삼각형으로 분할하여 인덱스 저장
			for (size_t i = 1; i + 1 < vertexCount; ++i)
			{
				m_subMeshes.back().indices.push_back(faceIndices[0]);
				m_subMeshes.back().indices.push_back(faceIndices[i]);
				m_subMeshes.back().indices.push_back(faceIndices[i + 1]);
			}
		}
		else if (prefix == "o")
		{
			lastprefixIsUsemtl = false;
			// 새 오브젝트 (서브메시) 시작
			SubMesh newSubMesh;
			m_subMeshes.emplace_back(newSubMesh);
		}
		else if (prefix == "usemtl")
		{
			if(lastprefixIsUsemtl == true)
				// 이전에 usemtl이 있었는데 또 있으면 새 서브메시 시작
				m_subMeshes.emplace_back(SubMesh());

			m_subMeshes.back().materialName.clear();

			std::string materialName;
			iss >> materialName;

			m_subMeshes.back().materialName = std::wstring(materialName.begin(), materialName.end());
			lastprefixIsUsemtl = true;
		}
    }

	// 각 서브메쉬를 돌면서 버퍼 생성 및 업로드
	for (auto& subMesh : m_subMeshes)
	{
		// 정점 버퍼 생성
		if (!subMesh.vertices.empty())
		{
			D3D11_BUFFER_DESC vbDesc = {};
			vbDesc.Usage = D3D11_USAGE_DEFAULT;
			vbDesc.ByteWidth = static_cast<UINT>(sizeof(MeshVertex) * subMesh.vertices.size());
			vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vbDesc.CPUAccessFlags = 0;
			D3D11_SUBRESOURCE_DATA vbData = {};
			vbData.pSysMem = subMesh.vertices.data();
			HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, subMesh.pVertexBuffer.GetAddressOf());
			if (FAILED(hr))
			{
				std::wcerr << L"Failed to create vertex buffer for sub-mesh in file: " << path << std::endl;
				return false;
			}
		}
		// 인덱스 버퍼 생성
		if (!subMesh.indices.empty())
		{
			D3D11_BUFFER_DESC ibDesc = {};
			ibDesc.Usage = D3D11_USAGE_DEFAULT;
			ibDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * subMesh.indices.size());
			ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			ibDesc.CPUAccessFlags = 0;
			D3D11_SUBRESOURCE_DATA ibData = {};
			ibData.pSysMem = subMesh.indices.data();
			HRESULT hr = device->CreateBuffer(&ibDesc, &ibData, subMesh.pIndexBuffer.GetAddressOf());
			if (FAILED(hr))
			{
				std::wcerr << L"Failed to create index buffer for sub-mesh in file: " << path << std::endl;
				return false;
			}
		}
	}

	file.close();

    return true;
}

MyEngine::Mesh::Mesh()
{

}

MyEngine::Mesh::~Mesh()
{
}

std::unique_ptr<MyEngine::Mesh> MyEngine::Mesh::CreateFromFile(ID3D11Device* device, std::wstring path)
{
	if (!device) {
		throw std::invalid_argument("Device is null");
	}

	auto mesh = std::make_unique<Mesh>();

	if (!mesh->LoadFromFile(device, path)) {
		throw std::runtime_error("Failed to load mesh from file");
	}

	return mesh; //RVNO 최적화 -> std::move(mesh);
}


HRESULT MyEngine::Mesh::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			auto cr = reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer());

			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}