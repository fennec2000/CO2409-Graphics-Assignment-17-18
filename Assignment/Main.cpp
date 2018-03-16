//--------------------------------------------------------------------------------------
// File: Main.cpp
//
// Window creation and messages, and the main render loop
//--------------------------------------------------------------------------------------

#include "Device.h"
#include "Shader.h"
#include "Scene.h"
#include "Timer.h" // Timer class - not DirectX
#include "Input.h" // Input functions - not DirectX
#include "Resource.h"
#include <windows.h>
#include <windowsx.h>

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	// These windows messages (WM_KEYXXXX) can be used to get keyboard input to the window
	// This application has added some simple functions (not DirectX) to process these messages (all in Input.cpp/h)
	case WM_KEYDOWN:
		KeyDownEvent(static_cast<EKeyCode>(wParam));
		break;
	case WM_KEYUP:
		KeyUpEvent(static_cast<EKeyCode>(wParam));
		break;

	// Get mouse buttons too, treat them as keys
	case WM_LBUTTONDOWN:
		KeyDownEvent(Mouse_LButton);
		break;
	case WM_LBUTTONUP:
		KeyUpEvent(Mouse_LButton);
		break;

	case WM_MBUTTONDOWN:
		KeyDownEvent(Mouse_MButton);
		break;
	case WM_MBUTTONUP:
		KeyUpEvent(Mouse_MButton);
		break;

	case WM_RBUTTONDOWN:
		KeyDownEvent(Mouse_RButton);
		break;
	case WM_RBUTTONUP:
		KeyUpEvent(Mouse_RButton);
		break;

	case WM_MOUSEMOVE:
		MouseMoveEvent(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
// Returns handle of created window or 0 on failure
HWND InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))  return 0;

	// Create window
	RECT rc = { 0, 0, 1280, 960 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 10: Parallax Mapping", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
	if (!hWnd)  return 0;

	ShowWindow(hWnd, nCmdShow);

	return hWnd;
}


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Initialise everything in turn
	HWND hWnd = InitWindow(hInstance, nCmdShow);
	if (hWnd == 0)  return 0;
	if (!InitDevice(hWnd) || !InitShaders() || !InitScene())
	{
		ReleaseScene();
		ReleaseShaders();
		ReleaseDevice();
		return 0;
	}

	// Initialise simple input functions (in Input.cpp) - not DirectX
	InitInput();

	// Initialise a timer class (in CTimer.h/.cpp, not part of DirectX). It's like a stopwatch - start it counting now
	Timer timer;
	timer.Start();

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		// First check to see if there are any messages that need to be processed for the window (window resizing, minimizing, whatever)
		// If not then the window is idle and the D3D rendering occurs. This is in a loop. So the window is rendered over and over, as fast as
		// possible as long as we are not manipulating the window in some way
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else // Otherwise render and update scene
		{
			RenderScene();

			// Get the time passed since the last frame (since the last time this line was reached) - used so the rendering and update can be
			// synchronised to real time and won't be dependent on machine speed
			float updateTime = timer.GetLapTime();

			UpdateScene(updateTime);

			// Allow user to quit with escape key
			if (KeyHit(Key_Escape)) 
			{
				DestroyWindow(hWnd);
			}
		}
	}

	// Release all the DirectX resources before leaving
	ReleaseScene();
	ReleaseShaders();
	ReleaseDevice();

	return (int)msg.wParam;
}
