#include "window.h"

LRESULT CALLBACK WindowProcess(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;

		switch (msg)
		{
		case WM_SIZE:
		{
			//if (wParam != SIZE_MINIMIZED)
			//{
			//	if (BLUE::g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
			//	{
			//		BLUE::WaitForLastSubmittedFrame();
			//		BLUE::CleanupRenderTarget();
			//		HRESULT result = BLUE::g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
			//		assert(SUCCEEDED(result) && "Failed to resize swapchain.");
			//		BLUE::CreateRenderTarget();
			//	}
			//}
		} return 0;

		case WM_SYSCOMMAND:
		{
			if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
		} break;

		case WM_DESTROY:
		{
			::PostQuitMessage(0);
		} return 0;

		}

		return ::DefWindowProcW(hwnd, msg, wParam, lParam);
	}

namespace blue
{
	Window::Window(uint32_t width, uint32_t height)
	{
		// Window data
		quit = false;
		windowHandler = nullptr;
		windowClass = {};
		windowWidth = width;
		windowHeight = height;
	}


	void Window::CreateHWindow()
	{
		// fill window parameters
		windowClass = { sizeof(windowClass), CS_CLASSDC, WindowProcess, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Blue Render Class", nullptr };
		if (!::RegisterClassExW(&windowClass))
		{
			MessageBox(nullptr, "Error registering class",
				"Error", MB_OK | MB_ICONERROR);
			return;
		}

		windowHandler = ::CreateWindowW(windowClass.lpszClassName, L"BlueD Render", WS_OVERLAPPEDWINDOW, 3000, 200, 1280, 800, nullptr, nullptr, windowClass.hInstance, nullptr);

		::ShowWindow(windowHandler, SW_SHOWDEFAULT);
		::UpdateWindow(windowHandler);
	}

	void Window::DestroyHWindow()
	{
		::DestroyWindow(windowHandler);
		::UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
	}
		
	// handle window event

}