#pragma once
#include <Windows.h>
#include <memory>
#include <Keyboard.h>
#include <Mouse.h>

namespace MyEngine 
{
	class Time;
	class MyD3DContext;
	class MyApp 
	{
	private:
		MyD3DContext* m_pD3DContext = nullptr;
		Time* m_pTime = nullptr;
		HINSTANCE m_hInst;
		HWND m_hWnd;
		int m_width = 1024;
		int m_height = 768;

		std::unique_ptr<DirectX::Keyboard> m_keyboard;
		std::unique_ptr<DirectX::Mouse> m_mouse;
	public:
		MyApp(HINSTANCE hInstance);
		~MyApp();

		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		int Run();
	};
}