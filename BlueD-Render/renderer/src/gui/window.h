#pragma once

#include <windows.h>
#include <cstdint>
#include <memory>

#include "backends/imgui_impl_win32.h"
#include "render.h"

namespace blue
{
	class Window
	{
	public:
		// window properties
		uint32_t		windowX = 350;
		uint32_t		windowY = 200;
		uint32_t		windowWidth;
		uint32_t		windowHeight;
		bool			quit;
		bool			initialized;
		bool			fullScreen = false;
		// winapi window variables
		HWND			windowHandler;
		WNDCLASSEXW		windowClass;

		// render object
		std::shared_ptr<blue::Render> render;

		void CreateHWindow();
		void SetRender(std::shared_ptr<blue::Render> r);
		void DestroyHWindow();

		Window();
		~Window();

	private:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT _WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
}