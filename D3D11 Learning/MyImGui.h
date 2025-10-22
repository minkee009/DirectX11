#pragma once
//#ifdef _DEBUG

#include <Windows.h>
#include <d3d11.h>

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
		void UpdateInfiniteDrag();
	public:
		bool Initialize(MyD3DContext* myContext);
		void BeginFrame();
		void Update();
		void Render();
		void Uninitialize();
	};
}

//#endif //_DEBUG