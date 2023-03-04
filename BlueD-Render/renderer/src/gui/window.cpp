#include "window.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace blue
{
	Window::Window(uint32_t x, uint32_t y, uint32_t width, uint32_t height) :
		windowX(x), windowY(y), windowWidth(width), windowHeight(height)
	{
		initialized = false;
		quit = false;
		windowHandler = nullptr;
		windowClass = {};
	}

	Window::~Window()
	{
		DestroyHWindow();
	}

	void Window::CreateHWindow()
	{
		// fill window parameters
		windowClass = { sizeof(windowClass), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Blue Render Class", nullptr };
		if (!::RegisterClassExW(&windowClass))
		{
			MessageBox(nullptr, "Error registering class",
				"Error", MB_OK | MB_ICONERROR);
			return;
		}

		if (fullScreen)
		{
			windowHandler = ::CreateWindowW(windowClass.lpszClassName, L"BlueD Render", WS_OVERLAPPEDWINDOW, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), nullptr, nullptr, windowClass.hInstance, this);
		}
		else
		{
			windowHandler = ::CreateWindowW(windowClass.lpszClassName, L"BlueD Render", WS_OVERLAPPEDWINDOW, windowX, windowY, 0, 0, nullptr, nullptr, windowClass.hInstance, this);
		}

		if (!windowHandler)
		{
			MessageBox(nullptr, "Error creating window",
				"Error", MB_OK | MB_ICONERROR);
			return;
		}

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
			if (render->g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED && initialized)
			{
				render->WaitForLastSubmittedFrame();
				render->CleanupRenderTarget();
				HRESULT result = render->g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
				assert(SUCCEEDED(result) && "Failed to resize swapchain.");
				render->CreateRenderTarget();
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