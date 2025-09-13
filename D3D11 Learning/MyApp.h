#pragma once
#include <Windows.h>

namespace MyEngine {
	class Time;
	class MyD3DContext;
	class MyApp {
	private:
		MyD3DContext* m_pD3DContext = nullptr;
		Time* m_pTime = nullptr;
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