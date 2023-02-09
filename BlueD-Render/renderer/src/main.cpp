#include "gui.h"

#include <iostream>
#include <thread>

int WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	// create gui
	blueGUI::CreateHWindow();
	if (!blueGUI::CreateDevice())
	{
		blueGUI::DestroyDevice();
		::UnregisterClassW(blueGUI::windowClass.lpszClassName, blueGUI::windowClass.hInstance);
		return 1;
	}
	ImGuiIO io = blueGUI::CreateGui();

	while (!blueGUI::quit)
	{
		blueGUI::StartRenderGui();
		blueGUI::RenderGui(io);

		//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// clean up
	blueGUI::DestroyGui();
	blueGUI::DestroyDevice();
	blueGUI::DestroyHWindow();

	return EXIT_SUCCESS;
}