#pragma once
#ifdef _DEBUG

#include <Windows.h>
#include <d3d11.h>

namespace MyEngine 
{
	class MyImGui
	{
	private:
		HWND m_hWnd;
		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pImmediateContext;

		float m_width;
		float m_height;

		bool m_isImGuiInit = false;
		bool m_isWin32BackendInit = false;
		bool m_isD3D11BackendInit = false;
	public:
		bool Initialize(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext);
		void BeginFrame();
		void Update();
		void Render();
		void Uninitialize();

		inline float GetWidth() { return m_width; }
		inline float GetHeight() { return m_height; }
		inline void SetWidth(float value) { m_width = value; }
		inline void SetHeight(float value) { m_height = value; }
	};
}

#endif //_DEBUG