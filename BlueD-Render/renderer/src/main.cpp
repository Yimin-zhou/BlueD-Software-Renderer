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

	gui.InitGui();

	while (!gui.quit)
	{
		// render gui
		gui.Render();
		// draw something
		dx.Render();
	}
	
	gui.CleanUp();
	dx.Cleanup();

	return EXIT_SUCCESS;
}