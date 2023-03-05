#pragma once

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"
#include "imgui_internal.h"

#include "render.h"

#include <memory>

namespace blue
{
	class Gui
	{
	public:
		std::shared_ptr<blue::Render>	render;
		HWND							windowHandler;

		void CreateGui();
		void DestroyGui();
		void SetDockspace();
		void AddGui();
		void NewGuiFrame();
		void RenderFrame();

		Gui(std::shared_ptr<blue::Render> r, HWND w);
		~Gui();
	};

}