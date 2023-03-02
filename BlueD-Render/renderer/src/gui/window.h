#pragma once

#include <windows.h>
#include <cstdint>

#include "imgui_impl_win32.h"

namespace blue
{
	class Window
	{
	public:
		// window properties
		uint32_t		windowWidth;
		uint32_t		windowHeight;
		bool			quit;
		// winapi window variables
		HWND			windowHandler;
		WNDCLASSEXW		windowClass;

		void CreateHWindow();
		void DestroyHWindow();

		Window(uint32_t width, uint32_t height);
		//~Window();

	private:

	};
}