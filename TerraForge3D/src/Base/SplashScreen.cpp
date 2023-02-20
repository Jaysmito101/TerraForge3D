#ifdef TERR3D_WIN32

#include <SplashScreen.h>
#include <resource.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <tchar.h>
#include <iostream>
#include <string>


namespace SplashScreen
{


HBITMAP hBitmap = NULL;
HINSTANCE hInstance;
HWND splashWindow;
HANDLE splashWindowThread;
static bool isRunning;

// For Future Use
static char splashMessage[256];
static int splashMessageLength;
static int commandPtr;



LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!isRunning)
	{
		DeleteObject(hBitmap);
		PostQuitMessage(0);
		return 0;
	}

	switch (message)
	{
		case WM_CREATE:
			srand((uint32_t)time(NULL));

			if(rand()%2==0)
			{
				hBitmap = (HBITMAP)LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(SPLASH1));
			}

			else
			{
				hBitmap = (HBITMAP)LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(SPLASH2));
			}

			break;

		case WM_PAINT:
			PAINTSTRUCT     ps;
			HDC             hdc;
			BITMAP          bitmap;
			HDC             hdcMem;
			HGDIOBJ         oldBitmap;
			hdc = BeginPaint(hwnd, &ps);
			hdcMem = CreateCompatibleDC(hdc);
			oldBitmap = SelectObject(hdcMem, hBitmap);
			GetObject(hBitmap, sizeof(bitmap), &bitmap);
			BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, oldBitmap);
			DeleteDC(hdcMem);
			/*
			            TextOutA(
			                hdc,
			                10,
			                305,
			                splashMessage,
			                splashMessageLength
			            );
			*/
			EndPaint(hwnd, &ps);
			break;

		case WM_DESTROY:
			DeleteObject(hBitmap);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

void SetSplashMessage(std::string message)
{
	strcpy_s(splashMessage, message.c_str());
	splashMessageLength = (int)message.size();
}

void ShowSplashScreen()
{
	std::cout << "Hsadgbshdy\n";
	commandPtr = 1;
}

void HideSplashScreen()
{
	commandPtr = 2;
}

DWORD WINAPI SetUpSplashWindow(LPVOID par)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
	std::string classname = "SplashWindowClass";
	//Step 1: Registering the Window Class
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = (LPCWSTR)classname.c_str();
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		//MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Step 2: Creating the Window
	hwnd = CreateWindowEx(
	           0,
	           (LPCWSTR)classname.c_str(),
	           L"The title of my window",
	           ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU),
	           GetSystemMetrics(SM_CXSCREEN) / 2 - 300,
	           GetSystemMetrics(SM_CYSCREEN) / 2 - 225,
	           600,
	           450,
	           NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		//MessageBox(NULL, L"Window Creation Failed!", L"Error!",
		//MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowScrollBar(hwnd, SB_BOTH, FALSE);
	ShowWindow(hwnd, TRUE);
	UpdateWindow(hwnd);
	splashWindow = hwnd;

	// Step 3: The Message Loop
	while (GetMessage(&Msg, NULL, 0, 0) && isRunning)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return 0;
}

void Init(HINSTANCE hIn)
{
	isRunning = true;
	commandPtr = 0;
	hInstance = hIn;
	memset(splashMessage, 0, 256);
	memcpy(splashMessage, (char *)"Loading...\0", 11);
	splashMessageLength = 10;
	splashWindowThread = CreateThread(NULL, 0, SetUpSplashWindow, (void *)0, 0, 0);
}
void Destory()
{
	isRunning = false;
	DestroyWindow(splashWindow);
	CloseWindow(splashWindow);
	CloseHandle(splashWindowThread);
}

}

#endif