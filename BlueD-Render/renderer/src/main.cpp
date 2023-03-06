#include "render.h"
#include "window.h"
#include "gui.h"

#include <iostream>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	// create window object
	std::shared_ptr <blue::Window> window = std::make_shared<blue::Window>();
	// create window
	window->CreateHWindow();

	// create render object
	std::shared_ptr<blue::Render> render = std::make_shared<blue::Render>(window->windowHandler);
	// create render device
	render->Init();

	// create GUI object
	std::unique_ptr<blue::Gui> gui = std::make_unique<blue::Gui>(render, window->windowHandler);

	window->SetRender(render);
	// can resize the window now
	window->initialized = true;

	// Main render loop
	MSG msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				break;
		}
		else
		{
			render->ResetCommandAllocator();
			// render gui
			gui->RenderFrame();
			// render main frame
			render->PopulateCommandList();
			render->RenderFrame();
		}
	}

	return EXIT_SUCCESS;
}