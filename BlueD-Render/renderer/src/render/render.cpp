#include "render.h"
#include "gui.h"

namespace blue
{
	
	Render::Render(HWND window) :
		g_pd3dDevice(nullptr),
		g_pAdapter(nullptr),
		g_pd3dRtvDescHeap(nullptr),
		g_pd3dSrvDescHeap(nullptr),
		g_pd3dCommandQueue(nullptr),
		g_pd3dCommandList(nullptr),
		g_fence(nullptr),
		g_fenceEvent(nullptr),
		g_fenceLastSignaledValue(0),
		g_pSwapChain(nullptr),
		g_hSwapChainWaitableObject(nullptr),
		g_pd3dRootSignature(nullptr),
		g_pd3dPipelineState(nullptr),
		g_pd3dVertexBuffer(nullptr),
		windowHandler(window)
	{
			clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	}

	Render::Render() :
		g_pd3dDevice(nullptr),
		g_pAdapter(nullptr),
		g_pd3dRtvDescHeap(nullptr),
		g_pd3dSrvDescHeap(nullptr),
		g_pd3dCommandQueue(nullptr),
		g_pd3dCommandList(nullptr),
		g_fence(nullptr),
		g_fenceEvent(nullptr),
		g_fenceLastSignaledValue(0),
		g_pSwapChain(nullptr),
		g_hSwapChainWaitableObject(nullptr),
		g_pd3dRootSignature(nullptr),
		g_pd3dPipelineState(nullptr),
		g_pd3dVertexBuffer(nullptr),
		windowHandler(nullptr)
	{
		clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	}

	Render::~Render()
	{
		DestroyDevice();	
	}

	void Render::CreateDevice()
	{
		UINT dxgiFactoryFlags = 0;
	#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();

				// Enable additional debug layers.
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}
	#endif

		// Create a factory for enumerating adapters and creating swap chains
		ComPtr<IDXGIFactory4> dxgiFactory4 = nullptr;
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory4)) != S_OK);

		// Enumerate adapters
		ComPtr<IDXGIAdapter1> dxgiAdapter1 = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory4->EnumAdapters1(adapterIndex, &dxgiAdapter1); adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			dxgiAdapter1->GetDesc1(&desc);
		
			// Check for hardware device
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
		
			// Check for WARP device
			if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
				continue;
		
			// Check for D3D12 support
			if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				break;
		}

		// Create the device
		ThrowIfFailed(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK);

		// Create command queue
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			ThrowIfFailed(g_pd3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(g_pd3dCommandQueue.GetAddressOf())) != S_OK);
		}

		// Create swap chain
		{
			DXGI_SWAP_CHAIN_DESC1 swapDesc;
			{
				ZeroMemory(&swapDesc, sizeof(swapDesc));
				swapDesc.BufferCount = NUM_BACK_BUFFERS;
				swapDesc.Width = 0;
				swapDesc.Height = 0;
				swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
				swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapDesc.SampleDesc.Count = 1;
				swapDesc.SampleDesc.Quality = 0;
				swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				swapDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
				swapDesc.Scaling = DXGI_SCALING_STRETCH;
				swapDesc.Stereo = FALSE;
			}

			ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
			ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(g_pd3dCommandQueue.Get(), windowHandler, &swapDesc, nullptr, nullptr, &swapChain1));
			ThrowIfFailed(swapChain1->QueryInterface(IID_PPV_ARGS(g_pSwapChain.GetAddressOf())));
			g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
			g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();

			ThrowIfFailed(swapChain1.As(&g_pSwapChain));
			g_frameIndex = g_pSwapChain->GetCurrentBackBufferIndex();
		}

		// Create RTV
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.NumDescriptors = NUM_BACK_BUFFERS;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeapDesc.NodeMask = 1;
			ThrowIfFailed(g_pd3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK);

			SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				g_mainRenderTargetDescriptor[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
			}
			// Create render target views
			CreateRenderTarget();
		}

		// Create SRV..
		{
			// The descriptor heap for the combination of constant-buffer, shader-resource, and unordered-access views.
			D3D12_DESCRIPTOR_HEAP_DESC csuDesc;
			csuDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			csuDesc.NumDescriptors = 1;
			csuDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(g_pd3dDevice->CreateDescriptorHeap(&csuDesc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK);
		}


		// Create command allocator and command lists
		for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
			ThrowIfFailed(g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK);


		// load assets
		// Create root Signature
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf()));
			ThrowIfFailed(g_pd3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(g_pd3dRootSignature.GetAddressOf())));
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

		#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		#else
			UINT compileFlags = 0;
		#endif
		
			ThrowIfFailed(D3DCompileFromFile(L"D:\\Programming\\CG\\BlueD-Software-Renderer\\BlueD-Render\\renderer\\src\\render\\Shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
			ThrowIfFailed(D3DCompileFromFile(L"D:\\Programming\\CG\\BlueD-Software-Renderer\\BlueD-Render\\renderer\\src\\render\\Shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

			// Define the vertex input layout.
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = g_pd3dRootSignature.Get();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			ThrowIfFailed(g_pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pd3dPipelineState)));
		}

		// Create command list
		ThrowIfFailed(g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator.Get(), nullptr, IID_PPV_ARGS(g_pd3dCommandList.GetAddressOf())) != S_OK ||
			g_pd3dCommandList->Close() != S_OK);

		// Create the vertex buffer.
		{
			// Define the geometry for a triangle.
			Vertex triangleVertices[] =
			{
				{ { 0.0f, 0.25f * 10.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
				{ { 0.25f, -0.25f * 10.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
				{ { -0.25f, -0.25f * 10.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
			};

			const UINT vertexBufferSize = sizeof(triangleVertices);

			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.
			ThrowIfFailed(g_pd3dDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(g_pd3dVertexBuffer.GetAddressOf())));

			// Copy the triangle data to the vertex buffer (GPU).
			UINT8* pVertexDataBegin;
			CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			ThrowIfFailed(g_pd3dVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
			g_pd3dVertexBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			g_pd3dVertexBufferView.BufferLocation = g_pd3dVertexBuffer->GetGPUVirtualAddress();
			g_pd3dVertexBufferView.StrideInBytes = sizeof(Vertex);
			g_pd3dVertexBufferView.SizeInBytes = vertexBufferSize;
		}

		// Create synchronization objects
		ThrowIfFailed(g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(g_fence.GetAddressOf())));
		for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
			g_frameContext[i].FenceValue = 1;

		// Create an event handle to use for frame synchronization.
		g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (g_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	void Render::DestroyDevice()
	{
		CleanupRenderTarget();

		//if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
		//if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
		//for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
		//	if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
		//if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
		//if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
		//if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
		//if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
		//if (g_fence) { g_fence->Release(); g_fence = nullptr; }
		//if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
		//if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

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
		//for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
		//	if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; } // Release the back buffer resource
	}

	void Render::WaitForLastSubmittedFrame()
	{
		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
		// sample illustrates how to use fences for efficient resource usage and to
		// maximize GPU utilization.

		FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

		// Signal and increment the fence value.
		UINT64 fenceValue = g_fenceLastSignaledValue + 1;
		g_pd3dCommandQueue->Signal(g_fence.Get(), fenceValue);
		g_fenceLastSignaledValue = fenceValue;

		// Wait until the previous frame is finished.
		if (g_fence->GetCompletedValue() < fenceValue)
		{
			ThrowIfFailed(g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent));
			WaitForSingleObject(g_fenceEvent, INFINITE);
		}

		g_frameIndex = g_pSwapChain->GetCurrentBackBufferIndex();
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

	void Render::PopulateCommandList()
	{
		// Wait for the previous frame to complete.
		FrameContext* frameCtx = WaitForNextFrameResources();

		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(frameCtx->CommandAllocator->Reset());
		ThrowIfFailed(g_pd3dCommandList->Reset(frameCtx->CommandAllocator.Get(), g_pd3dPipelineState.Get()));

		// Set necessary state.
		g_pd3dCommandList->SetGraphicsRootSignature(g_pd3dRootSignature.Get());
		g_pd3dCommandList->RSSetViewports(1, &g_viewport);
		g_pd3dCommandList->RSSetScissorRects(1, &g_scissorRect);

		// Indicate that the back buffer will be used as a render target.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = g_mainRenderTargetResource[g_frameIndex].Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		g_pd3dCommandList->ResourceBarrier(1, &barrier);

		g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[g_frameIndex], FALSE, nullptr);

		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[g_frameIndex], clearColor, 0, nullptr);
		g_pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g_pd3dCommandList->IASetVertexBuffers(0, 1, &g_pd3dVertexBufferView);

		// Draw
		g_pd3dCommandList->DrawInstanced(3, 1, 0, 0);

		g_pd3dCommandList->SetDescriptorHeaps(1, g_pd3dSrvDescHeap.GetAddressOf());
		//g_pd3dCommandList->SetDescriptorHeaps(1, g_pd3dRtvDescHeap.GetAddressOf());

		// Render ImGui
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList.Get());
		// Close befor execute
		ThrowIfFailed(g_pd3dCommandList->Close());
	}

	void Render::RenderFrame()
	{
		PopulateCommandList();
		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { g_pd3dCommandList.Get() };
		g_pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(nullptr, (void*)g_pd3dCommandList.Get());
		}

		// Present the frame.
		g_pSwapChain->Present(1, 0); // Present with vsync
	}
}