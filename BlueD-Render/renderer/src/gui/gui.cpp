#include "gui.h"


namespace blue
{
	Gui::Gui(std::shared_ptr<blue::Render> r) :
		render(r)
	{
		CreateGui();
	}

	Gui::~Gui()
	{
		DestroyGui();
	}

	void Gui::CreateGui()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
		// Load Fonts
		//io.Fonts->AddFontFromFileTTF("D:\\Programming\\CG\B\lueD-Software-Renderer\\BlueD-Render\\renderer\\resource\\font\\Roboto-Regular.ttf", 15.0f);
		//int width, height;
		//unsigned char* pixels = nullptr;
		//io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 5.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(render->windowHandler);
		ImGui_ImplDX12_Init(render->g_pd3dDevice, render->NUM_FRAMES_IN_FLIGHT,
			DXGI_FORMAT_R8G8B8A8_UNORM, render->g_pd3dSrvDescHeap,
			render->g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			render->g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
	}

	void Gui::DestroyGui()
	{
		// Cleanup
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void Gui::RenderFrame()
	{
		NewGuiFrame();
		SetDockspace();
		AddGui();
		ImGui::Render();
	}

	void Gui::NewGuiFrame()
	{
		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void Gui::SetDockspace()
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
	}

	static void ShowDebugOverlay(bool* p_open)
	{
		static int location = 0;
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		if (location >= 0)
		{
			const float PAD = 40.0f;
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size = viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
			window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
			window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
			window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			ImGui::SetNextWindowViewport(viewport->ID);
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		else if (location == -2)
		{
			// Center window
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		ImGui::SetNextWindowBgAlpha(0.50f); // Transparent background
		if (ImGui::Begin("Debug", p_open, window_flags))
		{
			// show fps
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
		}
		ImGui::End();
	}

	static void ShowViewport(IDXGISwapChain3* g_pSwapChain)
	{
		// TODO: a viewport camera that change with the below viewport.
		ImGuiIO& io = ImGui::GetIO();
		// create a viewport to show a dx12 render target, and make the render target fill and resize with viewport.
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();

		ImVec2 viewport_size = ImVec2(viewport_panel_size.y * 16 / 9, viewport_panel_size.y);

		ImGui::Image((void*)g_pSwapChain->GetCurrentBackBufferIndex(), viewport_size, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
	}

	static void ShowContent()
	{
		// TODO: Create a content browser, where user can drag and drop assets.
		ImGui::Begin("Content Browser");
		ImGui::End();
	}

	static void ShowHierarchy()
	{
		// TODO:Create a scene hierarchy, where user can drag and drop game objects.
		ImGui::Begin("Scene Hierarchy");
		ImGui::End();
	}

	void Gui::AddGui()
	{
		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		ImGui::ShowDemoWindow();
		
		// show fps
		bool showDebugOverlay = true;
		ShowDebugOverlay(&showDebugOverlay);

		// 2. Create a window as viewport for render
		ShowViewport(render->g_pSwapChain);

		// 3. Create a window as content browser
		ShowContent();

		// 4. Create a window as scene hierarchy
		ShowHierarchy();
	}
}
