#include "Time.h"
#include "MyApp.h"
#include "MyD3DContext.h"
#include <wrl/client.h>

#ifdef _DEBUG
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif //_DEBUG

MyEngine::MyApp::MyApp(HINSTANCE hInstance)
{
	m_hInst = hInstance;
	// 윈도우 클래스 정의
	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.lpfnWndProc = StaticWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"MyEngineClass";
	RegisterClassEx(&wc);

	// 클라이언트 영역에 맞춰 윈도우 전체 크기 계산
	RECT clientRect = { 0, 0, m_width, m_height };
	AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);

	// 윈도우 생성
	m_hWnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		L"My 3D Engine",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		clientRect.right - clientRect.left,
		clientRect.bottom - clientRect.top,
		NULL,
		NULL,
		hInstance,
		this
	);
	ShowWindow(m_hWnd, SW_SHOW);

	m_pTime = new Time();

	// Direct3D 컨텍스트 초기화
	m_pD3DContext = new MyD3DContext();
	m_pD3DContext->Initialize(m_hWnd, m_width, m_height);
	bool as = m_pD3DContext->InitializeScene();

	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<Mouse>();

	Mouse::Get().SetWindow(m_hWnd);
}

MyEngine::MyApp::~MyApp()
{
	if (m_pTime) {
		delete m_pTime;
		m_pTime = nullptr;
	}

	if (m_pD3DContext) {
		m_pD3DContext->UninitializeScene();

		delete m_pD3DContext;
		m_pD3DContext = nullptr;
	}

	if (m_hWnd) {
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
}

LRESULT MyEngine::MyApp::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCCREATE) {
		CREATESTRUCT* pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCS->lpCreateParams));
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	MyApp* pApp = reinterpret_cast<MyApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pApp) {
		return pApp->HandleMessage(hWnd, message, wParam, lParam);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT MyEngine::MyApp::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
#endif //_DEBUG

	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		if (m_pD3DContext && (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED || wParam == SIZE_MINIMIZED)) {
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			UINT newWidth = clientRect.right - clientRect.left;
			UINT newHeight = clientRect.bottom - clientRect.top;

			// 리사이즈 로직을 D3DContext에 위임
			if (newWidth > 0 && newHeight > 0) {
				m_pD3DContext->Resize(newWidth, newHeight);
			}
		}
		break;
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
		DirectX::Mouse::ProcessMessage(message, wParam, lParam);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		{
			// This is where you'd implement the classic ALT+ENTER hotkey for fullscreen toggle
			// 알트 엔터 이벤트 처리
		}
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
		break;
	case WM_MENUCHAR:
		// A menu is active and the user presses a key that does not correspond
		// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		DirectX::Mouse::ProcessMessage(message, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int MyEngine::MyApp::Run()
{
	if (FAILED(CoInitialize(nullptr)))
		return -1;

	MSG msg = {};

	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			m_pTime->Update();
			m_pD3DContext->Render();
		}
	}

	CoUninitialize();
	return (int)msg.wParam;
}
