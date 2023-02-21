#include "gui.h"

#include <iostream>
#include <thread>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{

	// create gui object
	Gui gui;
	gui.InitWindow();

	// create dx12 object
	DXBlue dx = DXBlue(gui.windowWidth, gui.windowHeight, gui.window);
	// initialize dx before initialize ImGui
	dx.Init();

	//gui.dx = &dx;
	//gui.InitGui();

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// render gui
			//gui.Render();
			dx.Render();
		}
	}
	
	//gui.CleanUp();
	dx.Cleanup();

	return EXIT_SUCCESS;
}