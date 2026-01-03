#pragma once
//#ifdef _DEBUG

#include <Windows.h>
#include <psapi.h>
#include <d3d11.h>
#include <wrl/client.h> 
#include <dxgi1_3.h>
#include <dxgi1_4.h>

using Microsoft::WRL::ComPtr;

namespace DebugStatusUI
{
	
	struct DRamDebugData
	{
		bool isValidData = false;
		SIZE_T workingSet;
		SIZE_T privateBytes;
		SIZE_T PagefileUsage;
		SYSTEM_INFO si;
		SIZE_T pageSize;
	};
	
	struct GPURamDebugData
	{
		bool isValidData = false;
		double usage;
		double page;
	};

	DRamDebugData GetCpuMemoryUsage();
	GPURamDebugData QueryGpuMemory(ComPtr<ID3D11Device> d3dDevice);
}

struct ImFont;
namespace MyEngine 
{
	class MyD3DContext;
	class MyImGui
	{
	private:
		MyD3DContext* m_d3dContext;
		HWND m_hWnd;
		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pImmediateContext;

		bool m_isImGuiInit = false;
		bool m_isWin32BackendInit = false;
		bool m_isD3D11BackendInit = false;
		bool m_isHovered = false;
		void UpdateInfiniteDrag();

		ImFont* m_bigFont;
		ImFont* m_defaultFont;
	public:
		bool Initialize(MyD3DContext* myContext);
		void BeginFrame();
		void Update();
		void Render();
		void Uninitialize();
		bool GetIsHovered();
	};
}

//#endif //_DEBUG