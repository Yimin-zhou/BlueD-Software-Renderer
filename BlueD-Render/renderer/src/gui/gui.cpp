#include "gui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// handle window event
LRESULT WINAPI WindowProcess(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;

	switch (msg)
	{
	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
	} break;

	case WM_DESTROY:
	{
		::PostQuitMessage(0);
	} return 0;

	}

	return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}


// Window data
bool		show_demo_window = false;

Gui::Gui()
{
}

Gui::~Gui()
{
}

void Gui::InitWindow()
{
	CreateHWindow();
}

void Gui::InitGui()
{
	CreateGui();
}

void Gui::Render()
{
	StartRenderGui();
	RenderGui();
}

void Gui::CleanUp()
{
	CleanUpHWindow();
	CleanUpGui();
}

void Gui::CreateHWindow()
{
	// fill window parameters
	windowClass = { sizeof(windowClass), CS_CLASSDC, WindowProcess, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"BlueD Render Class", nullptr };
	if(!::RegisterClassExW(&windowClass))
	{
		MessageBox(nullptr, "Error registering class",
			"Error", MB_OK | MB_ICONERROR);
		return;
	}

	window = ::CreateWindowW(windowClass.lpszClassName, L"BlueD Render", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, windowClass.hInstance, nullptr);

	::ShowWindow(window, SW_SHOWDEFAULT);
	::UpdateWindow(window);
}

void Gui::CleanUpHWindow()
{
	::DestroyWindow(window);
	::UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
}

void Gui::CreateGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	io = ImGui::GetIO(); (void)io;
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
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX12_Init(dx->g_pDevice.Get(), dx->g_FrameCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, dx->g_pSrvHeap.Get(),
		dx->g_pSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		dx->g_pSrvHeap->GetGPUDescriptorHandleForHeapStart());
}

void Gui::CleanUpGui()
{
	dx->WaitForPreviousFrame();

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Gui::StartRenderGui()
{
	// Poll and handle messages (inputs, window resize, etc.)
	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		if (msg.message == WM_QUIT)
			quit = true;
	}
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Gui::RenderGui()
{
	// dockspace
	{
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static bool p_open = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &p_open, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Options"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows,
				// which we can't undo at the moment without finer window depth/z control.
				ImGui::MenuItem("Fullscreen", nullptr, &opt_fullscreen);
				ImGui::MenuItem("Padding", nullptr, &opt_padding);
				ImGui::Separator();

				if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
				if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
				if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
				if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
				if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
				ImGui::Separator();

				if (ImGui::MenuItem("Close", nullptr, false, &p_open != nullptr))
					p_open = false;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::End();
	}

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show Debug window
	{
		ImGui::Begin("Debug: ");                          

		ImGui::Text("This is some useful text.");               
		ImGui::Checkbox("Demo Window", &show_demo_window);      

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// Rendering
	ImGui::Render();

	// DX12
	uint32_t nextFrameIndex = dx->WaitForNextFrameResources();
	ID3D12CommandAllocator* tempCommandAllocator = dx->g_pCommandAllocator[nextFrameIndex].Get();
	uint64_t tempFenceValue = dx->g_pFenceValue[nextFrameIndex];
	uint32_t backBufferIdx = dx->g_pSwapChain->GetCurrentBackBufferIndex();
	tempCommandAllocator->Reset();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = dx->g_SwapChainBuffer[backBufferIdx].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	dx->g_pCommandList->Reset(tempCommandAllocator, nullptr);
	dx->g_pCommandList->ResourceBarrier(1, &barrier);

	// Render Dear ImGui graphics
	// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dx->g_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), dx->g_pFrameIndex, dx->g_rtvDescriptorSize);
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	dx->g_pCommandList->ClearRenderTargetView(rtvHandle, clear_color_with_alpha, 0, nullptr);
	dx->g_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	dx->g_pCommandList->SetDescriptorHeaps(1, &dx->g_pSrvHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx->g_pCommandList.Get());
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	dx->g_pCommandList->ResourceBarrier(1, &barrier);
	dx->g_pCommandList->Close();

	//dx->g_pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)dx->g_pCommandList.Get());

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)dx->g_pCommandList.Get());
	}

	dx->g_pSwapChain->Present(1, 0); // Present with vsync
	//g_pSwapChain->Present(0, 0); // Present without vsync

	UINT64 fenceValue = dx->g_fenceLastSignaledValue[dx->g_pFrameIndex] + 1;
	dx->g_pCommandQueue->Signal(dx->g_pFence[dx->g_pFrameIndex].Get(), fenceValue);
	dx->g_fenceLastSignaledValue[dx->g_pFrameIndex] = fenceValue;
	dx->g_pFenceValue[dx->g_pFrameIndex] = fenceValue;
}
