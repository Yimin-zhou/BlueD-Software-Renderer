#include "window.h"
#include "settings.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace blue
{
	Window::Window() :
		windowX(setting::windowPosX), windowY(setting::windowPosY), windowWidth(setting::windowWidth), windowHeight(setting::windowHeight), render(nullptr), windowHandler(nullptr)
	{
		initialized = false;
		quit = false;
		windowClass = {};
	}

	Window::~Window()
	{
		DestroyHWindow();
	}

	void Window::CreateHWindow()
	{
		const wchar_t CLASS_NAME[] = L"BlueRenderWindowClass";
		const wchar_t WINDOW_TITLE[] = L"Blue Render";

		// register window class
		windowClass.cbSize = sizeof(WNDCLASSEXW);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = GetModuleHandle(NULL);
		windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = CLASS_NAME;
		windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
		::RegisterClassExW(&windowClass);

		// Get the dimensions of the screen
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		windowX = (screenWidth - windowWidth) / 2;
		windowY = (screenHeight - windowHeight) / 2;

		// create window
		windowHandler = ::CreateWindowExW(0, windowClass.lpszClassName, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, windowX, windowY, windowWidth, windowHeight, NULL, NULL, windowClass.hInstance, this);

		if (!windowHandler)
		{
			MessageBox(nullptr, "Error creating window",
				"Error", MB_OK | MB_ICONERROR);
			return;
		}

		// show window
		::ShowWindow(windowHandler, SW_SHOWDEFAULT);
		::UpdateWindow(windowHandler);
	}

	void Window::SetRender(std::shared_ptr<blue::Render> r)
	{
		render = r;
	}

	void Window::DestroyHWindow()
	{
		::DestroyWindow(windowHandler);
		::UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
	}

	LRESULT WINAPI Window::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (WM_NCCREATE == msg)
		{
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
			return TRUE;
		}

		return ((Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA))->_WindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT Window::_WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{

		case WM_SIZE:
			if (render != nullptr && render->m_device != nullptr && wParam != SIZE_MINIMIZED && initialized)
			{
				//render->OnResize(lParam);
			}

			return 0;
		
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		}
		return ::DefWindowProcW(hWnd, msg, wParam, lParam);
	}
}