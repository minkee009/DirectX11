#include "MyApp.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	MyEngine::MyApp app(hInstance);
	return app.Run();
}


//디버그용 진입점
#ifdef _DEBUG
int main()
{
	MyEngine::MyApp app(GetModuleHandle(NULL));
	return app.Run();
}
#endif