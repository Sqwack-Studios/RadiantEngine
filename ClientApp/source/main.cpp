//  Filename: main.cpp
//	Author:	Daniel														
//	Date: 17/04/2025 21:00:06		
//  Sqwack-Studios													

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#include "RadiantEngine/core/types.h"

using namespace RE;

static constexpr fp32 ASPECT_RATIO{ 16.f / 9.f};
static constexpr int16 WINDOW_HEIGHT{ 1080 };
static constexpr int16 WINDOW_WIDTH{ static_cast<int16>(WINDOW_HEIGHT * ASPECT_RATIO) };

static HINSTANCE instance;

static constexpr wchar_t WINDOW_NAME[]{ L"FedeGordo" };


//process messages from kernel
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int main(int argc, char** argv)
{

	HINSTANCE instance{ ::GetModuleHandle(NULL) };

	WNDCLASS windowclass{
		.style = 0,
		.lpfnWndProc = WndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = instance,
		.hIcon = NULL,
		.hCursor = NULL,
		.hbrBackground = NULL,
		.lpszClassName = WINDOW_NAME
	};
	
	::RegisterClass(&windowclass);

	HWND hWnd{ ::CreateWindowExW(0, WINDOW_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, instance, NULL) };

	::ShowWindow(hWnd, SW_SHOW);

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}


	return 0;
}



