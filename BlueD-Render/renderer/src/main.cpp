#include "render.h"
#include "window.h"
#include "gui.h"

#include <iostream>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	// create window object
	std::shared_ptr <blue::Window> window = std::make_shared<blue::Window>(350, 200, 800, 500);
	// create window
	window->CreateHWindow();

	// create render object
	std::shared_ptr<blue::Render> render = std::make_shared<blue::Render>(window->windowHandler);
	// create render device
	render->CreateDevice();

	window->SetRender(render);
	// can resize the window now
	window->initialized = true;

	// create GUI object
	std::unique_ptr<blue::Gui> gui = std::make_unique<blue::Gui>(render, window->windowHandler);

	while (!window->quit)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		{
			MSG msg;
			while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				if (msg.message == WM_QUIT)
					window->quit = true;
			}
		}

		render->ResetCommandAllocator();
		// render gui
		gui->RenderFrame();
		// render main frame
		render->PopulateCommandList();
		render->RenderFrame();
	}

	return EXIT_SUCCESS;
}