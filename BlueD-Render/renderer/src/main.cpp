#include "render.h"
#include "window.h"

#include <iostream>
#include <thread>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	// create window object
	blue::Window window = blue::Window(800, 500);
	// create window
	window.CreateHWindow();

	// create render object
	blue::Render render = blue::Render(window.windowHandler);
	// create render device
	if (!render.CreateDevice())
	{
		render.DestroyDevice();
		::UnregisterClassW(window.windowClass.lpszClassName, window.windowClass.hInstance);
		return 1;
	}
	// create render GUI
	render.CreateGui();

	while (!window.quit)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		{
			MSG msg;
			while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				if (msg.message == WM_QUIT)
					window.quit = true;
			}
		}

		// render gui
		render.StartNewGuiFrame();
		render.RenderGui();
	}

	// clean up
	render.DestroyGui();
	render.DestroyDevice();
	window.DestroyHWindow();

	return EXIT_SUCCESS;
}