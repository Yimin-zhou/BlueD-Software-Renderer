#include "render.h"
#include "window.h"
#include "gui.h"

#include <iostream>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	// render object pointer
	std::shared_ptr<blue::Render> render = std::make_shared<blue::Render>();

	// create window object
	blue::Window window = blue::Window(350, 200, 800, 500);
	// create window
	window.SetRender(render);
	window.CreateHWindow();

	// create render object
	render = std::make_shared<blue::Render>(window.windowHandler);
	// create render device
	if (!render->CreateDevice())
	{
		render->DestroyDevice();
		return 1;
	}

	// can resize the window now
	window.initialized = true;

	// create GUI object
	std::shared_ptr<blue::Gui> gui = std::make_shared<blue::Gui>(render);

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
		gui->RenderFrame();

		// render main frame
		render->RenderFrame();
	}

	return EXIT_SUCCESS;
}