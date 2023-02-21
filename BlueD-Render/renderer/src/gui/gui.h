#pragma once

#include <windows.h> 
#include <d3d12.h>
#include <dxgi1_4.h>
#include <cstdint>
#include <wrl/client.h>

#include "dx/dx_blue.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"


class Gui
{
public:
	// Windows window
	bool					quit = false;
	const uint32_t			windowWidth = 800;
	const uint32_t			windowHeight = 500;
	// winapi window variables
	HWND					window = nullptr;
	WNDCLASSEXW				windowClass = {};

	// points for window movement
	POINTS					position;
	ImVec4					clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);;
	// ImGui IO
	ImGuiIO					io;
	// DX12 object
	DXBlue*					dx = nullptr;

	void InitWindow();
	void InitGui();
	void Render();
	void CleanUp();

	void CreateHWindow();
	void CleanUpHWindow();

	void CreateGui();
	void CleanUpGui();

	void StartRenderGui();
	void RenderGui();

	Gui();
	~Gui();

};