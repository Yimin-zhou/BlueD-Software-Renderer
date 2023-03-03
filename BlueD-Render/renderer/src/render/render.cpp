#include "render.h"
#include "gui.h"

namespace blue
{
	Render::Render(HWND window) :
		g_pd3dDevice(nullptr),
		g_pd3dRtvDescHeap(nullptr),
		g_pd3dSrvDescHeap(nullptr),
		g_pd3dCommandQueue(nullptr),
		g_pd3dCommandList(nullptr),
		g_fence(nullptr),
		g_fenceEvent(nullptr),
		g_fenceLastSignaledValue(0),
		g_pSwapChain(nullptr),
		g_hSwapChainWaitableObject(nullptr),
		windowHandler(window)
	{
			clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	}

	Render::Render() :
		g_pd3dDevice(nullptr),
		g_pd3dRtvDescHeap(nullptr),
		g_pd3dSrvDescHeap(nullptr),
		g_pd3dCommandQueue(nullptr),
		g_pd3dCommandList(nullptr),
		g_fence(nullptr),
		g_fenceEvent(nullptr),
		g_fenceLastSignaledValue(0),
		g_pSwapChain(nullptr),
		g_hSwapChainWaitableObject(nullptr),
		windowHandler(nullptr)
	{
		clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	}

	Render::~Render()
	{
		DestroyDevice();	
	}

	bool Render::CreateDevice()
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC1 sd;
		{
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = NUM_BACK_BUFFERS;
			sd.Width = 0;
			sd.Height = 0;
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			sd.Scaling = DXGI_SCALING_STRETCH;
			sd.Stereo = FALSE;
		}

		// [DEBUG] Enable debug interface
	#ifdef DX12_ENABLE_DEBUG_LAYER
		ID3D12Debug* pdx12Debug = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
			pdx12Debug->EnableDebugLayer();
	#endif

		// Create device
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
			return false;

		// [DEBUG] Setup debug interface to break on any warnings/errors
	#ifdef DX12_ENABLE_DEBUG_LAYER
		if (pdx12Debug != nullptr)
		{
			ID3D12InfoQueue* pInfoQueue = nullptr;
			g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			pInfoQueue->Release();
			pdx12Debug->Release();
		}
	#endif

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = NUM_BACK_BUFFERS;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			desc.NodeMask = 1;
			if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
				return false;

			SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				g_mainRenderTargetDescriptor[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
			}
		}

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
				return false;
		}

		{
			// Create command queue
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.NodeMask = 1;
			if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
				return false;
		}

		// Create command allocator and command lists
		for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
			if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
				return false;

		// Create command list
		if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
			g_pd3dCommandList->Close() != S_OK)
			return false;

		// Create fence
		if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
			return false;

		// Create fence event
		g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (g_fenceEvent == nullptr)
			return false;

		{
			// Create swap chain
			IDXGIFactory4* dxgiFactory = nullptr;
			IDXGISwapChain1* swapChain1 = nullptr;
			if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
				return false;
			if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, windowHandler, &sd, nullptr, nullptr, &swapChain1) != S_OK)
				return false;
			if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
				return false;
			swapChain1->Release(); 
			dxgiFactory->Release(); 
			g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS); 
			g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject(); 
		}

		// Create render target views
		CreateRenderTarget();

		return true;
	}

	void Render::DestroyDevice()
	{
		CleanupRenderTarget();

		if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
		if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
		for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
			if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
		if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
		if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
		if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
		if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
		if (g_fence) { g_fence->Release(); g_fence = nullptr; }
		if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
		if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

	#ifdef DX12_ENABLE_DEBUG_LAYER
		IDXGIDebug1* pDebug = nullptr;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
		{
			pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
			pDebug->Release();
		}
	#endif
	}

	void Render::CreateRenderTarget()
	{
		// Create a RTV for each frame.
		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
		{
			ID3D12Resource* pBackBuffer = nullptr;
			g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer)); 
			g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
			g_mainRenderTargetResource[i] = pBackBuffer; // Store the back buffer resource for later usage
		}
	}

	void Render::CleanupRenderTarget()
	{
		WaitForLastSubmittedFrame();

		// Release the render target views.
		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; } // Release the back buffer resource
	}

	void Render::WaitForLastSubmittedFrame()
	{
		FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue == 0)
			return; // No fence was signaled

		frameCtx->FenceValue = 0;
		if (g_fence->GetCompletedValue() >= fenceValue)
			return;

		g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	FrameContext* Render::WaitForNextFrameResources()
	{
		UINT nextFrameIndex = g_frameIndex + 1;
		g_frameIndex = nextFrameIndex;

		HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, nullptr };
		DWORD numWaitableObjects = 1;

		FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
		UINT64 fenceValue = frameCtx->FenceValue;

		// If the next frame is not ready to be rendered yet, wait until it is ready.
		if (fenceValue != 0) // means no fence was signaled
		{
			frameCtx->FenceValue = 0;
			g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
			waitableObjects[1] = g_fenceEvent;
			numWaitableObjects = 2;
		}

		// Wait until all previous GPU work is complete.
		WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

		return frameCtx;
	}

	void Render::RenderFrame()
	{
		// DX12
		{
			// Wait for the previous frame to complete.
			FrameContext* frameCtx = WaitForNextFrameResources();
			UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
			frameCtx->CommandAllocator->Reset();

			// Transition the frame to be used as a render target.
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
			g_pd3dCommandList->ResourceBarrier(1, &barrier);

			// Render Dear ImGui graphics
			const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
			g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
			g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
			g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			g_pd3dCommandList->ResourceBarrier(1, &barrier);
			g_pd3dCommandList->Close();

			// TODO: Render a Triangle.
			

			// Execute the command list.
			g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

			// Update and Render additional Platform Windows
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault(nullptr, (void*)g_pd3dCommandList);
			}

			// Present the frame.
			g_pSwapChain->Present(1, 0); // Present with vsync
			//g_pSwapChain->Present(0, 0); // Present without vsync

			// Signal and increment the fence value.
			UINT64 fenceValue = g_fenceLastSignaledValue + 1;
			g_pd3dCommandQueue->Signal(g_fence, fenceValue);
			g_fenceLastSignaledValue = fenceValue;
			frameCtx->FenceValue = fenceValue;
		}
	}
}