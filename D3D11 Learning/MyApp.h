#pragma once
#include <Windows.h>

namespace MyEngine {
	class MyD3DContext;
	class MyApp {
	private:
		MyD3DContext* m_d3dContext = nullptr;
		HINSTANCE m_hInst;
		HWND m_hWnd;
		int m_width = 800;
		int m_height = 600;
	public:
		MyApp(HINSTANCE hInstance);
		~MyApp();
		int Run();
	};
}