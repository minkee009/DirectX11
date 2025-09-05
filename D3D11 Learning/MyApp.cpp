#include "MyApp.h"
#include "MyD3DContext.h"
#include <wrl/client.h>

MyEngine::MyApp::MyApp(HINSTANCE hInstance)
{
	m_hInst = hInstance;
	// ������ Ŭ���� ����
	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
		switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
		};
	wc.hInstance = hInstance;
	wc.lpszClassName = L"MyEngineClass";
	RegisterClassEx(&wc);
	// ������ ����
	m_hWnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		L"My 3D Engine",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	ShowWindow(m_hWnd, SW_SHOW);

	// Direct3D ���ؽ�Ʈ �ʱ�ȭ (���� �ʿ�)
	m_d3dContext = new MyD3DContext(); // ��: Direct3D 11 ���ؽ�Ʈ
	m_d3dContext->Initialize(m_hWnd, m_width, m_height);
	bool as = m_d3dContext->InitializeScene();
}

MyEngine::MyApp::~MyApp()
{
	if (m_d3dContext) {
		m_d3dContext->UninitializeScene();

		delete m_d3dContext;
		m_d3dContext = nullptr;
	}

	if (m_hWnd) {
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
}

int MyEngine::MyApp::Run()
{
	MSG msg = {};
	CoInitialize(nullptr);
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			m_d3dContext->Render();
		}
	}
	CoUninitialize();
	return (int)msg.wParam;
}
