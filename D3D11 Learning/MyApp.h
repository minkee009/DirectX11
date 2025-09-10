#pragma once
#include <Windows.h>

namespace MyEngine {
	class MyD3DContext;
	class MyApp {
	private:
		MyD3DContext* m_pD3DContext = nullptr;
		HINSTANCE m_hInst;
		HWND m_hWnd;
		int m_width = 1024;
		int m_height = 768;
	public:
		MyApp(HINSTANCE hInstance);
		~MyApp();

		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		int Run();
	};
}