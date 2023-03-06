#include "render.h"
#include "gui.h"
#include "settings.h"

namespace blue
{

	Render::Render(HWND window) :
		m_device(nullptr),
		m_adapter(nullptr),
		m_swapChainRtvDescHeap(nullptr),
		m_srvImGuiDescHeap(nullptr),
		m_commandQueue(nullptr),
		m_commandList(nullptr),
		m_fence(nullptr),
		m_fenceEvent(nullptr),
		m_fenceValue(0),
		m_swapChain(nullptr),
		m_swapChainWaitableObject(nullptr),
		m_rootSignature(nullptr),
		m_pipelineState(nullptr),
		m_vertexBuffer(nullptr),
		windowHandler(window),
		m_frameIndex(0),
		m_viewport(0.0f, 0.0f, (float)setting::windowWidth, (float)setting::windowHeight),
		m_scissorRect(0, 0, setting::windowWidth, setting::windowHeight)
	{
		clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	}

	Render::Render() :
		m_device(nullptr),
		m_adapter(nullptr),
		m_swapChainRtvDescHeap(nullptr),
		m_srvImGuiDescHeap(nullptr),
		m_commandQueue(nullptr),
		m_commandList(nullptr),
		m_fence(nullptr),
		m_fenceEvent(nullptr),
		m_fenceValue(0),
		m_swapChain(nullptr),
		m_swapChainWaitableObject(nullptr),
		m_rootSignature(nullptr),
		m_pipelineState(nullptr),
		m_vertexBuffer(nullptr),
		windowHandler(nullptr),
		m_frameIndex(0),
		m_viewport(0.0f, 0.0f, (float)setting::windowWidth, (float)setting::windowHeight),
		m_scissorRect(0, 0, setting::windowWidth, setting::windowHeight)
	{
		clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	}

	Render::~Render()
	{
		Cleanup();
	}

	void Render::Init()
	{
		LoadPipeline();
		LoadAsset();
	}

	void Render::Cleanup()
	{
		_WaitForLastSubmittedFrame();

	#ifdef DX12_ENABLE_DEBUG_LAYER
		IDXGIDebug1* pDebug = nullptr;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
		{
			pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
			pDebug->Release();
		}
	#endif
	}

	void Render::LoadPipeline()
	{
		_CreateDevice();
		_CreateCommandQueue();
		_CreateSwapchain();
		_CreateRTV();
		_CreateDSV();
		_CreateImGuiSRV();
		_CreateCmdAllocator();
	}

	void Render::LoadAsset()
	{
		_CreateRootSig();
		_CreatePSO();
		_CreateCommandList();
		_CreateVertexBuffer();
		_CreateSynchronization();
	}

	void Render::_CreateDevice()
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
		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_dxgiFactory4.GetAddressOf())), "Creating DXGI Factory failed");

		// Enumerate adapters
		ComPtr<IDXGIAdapter1> dxgiAdapter1 = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory4->EnumAdapters1(adapterIndex, &dxgiAdapter1); adapterIndex++)
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
			if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
				break;
		}

		// Create the device
		ThrowIfFailed(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)), "Creating DX12 Device failed");

		// Cheack for MSAA support
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
		msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		msQualityLevels.SampleCount = 4;
		msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msQualityLevels.NumQualityLevels = 0;
		ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)), "Checking MSAA support failed");
	}

	void Render::_CreateCommandQueue()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf())), "Creating Command Queue failed");
	}

	void Render::_CreateCommandList()
	{
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.GetAddressOf())) != S_OK ||
			m_commandList->Close() != S_OK, "Creating Command List failed");
	}

	void Render::_CreateCmdAllocator()
	{
		// Create command allocator
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.GetAddressOf())), "Creating Command Allocator failed");
	}

	void Render::_CreateSwapchain()
	{
		DXGI_SWAP_CHAIN_DESC1 swapDesc;
		{
			ZeroMemory(&swapDesc, sizeof(swapDesc));
			swapDesc.BufferCount = NUM_BACK_BUFFERS;
			swapDesc.Width = setting::windowWidth;
			swapDesc.Height = setting::windowHeight;
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
		ThrowIfFailed(m_dxgiFactory4->CreateSwapChainForHwnd(m_commandQueue.Get(), windowHandler, &swapDesc, nullptr, nullptr, &swapChain1), "Creating Swapchain failed");
		ThrowIfFailed(swapChain1->QueryInterface(IID_PPV_ARGS(m_swapChain.GetAddressOf())), "Creating SwapChain QueryInterface failed");
		m_swapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
		m_swapChainWaitableObject = m_swapChain->GetFrameLatencyWaitableObject();

		ThrowIfFailed(swapChain1.As(&m_swapChain), "Creating Copying Swapchain failed");
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void Render::_CreateRTV()
	{
		// Create RTV
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.NumDescriptors = NUM_BACK_BUFFERS;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeapDesc.NodeMask = 1;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_swapChainRtvDescHeap)), "Creating Main RTV Heap Desc failed");

			SIZE_T rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_swapChainRtvDescHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				m_swapChainRTResourceDescHandle[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
			}
			// Create a RTV for each frame.
			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				ID3D12Resource* pBackBuffer = nullptr;
				m_swapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
				m_device->CreateRenderTargetView(pBackBuffer, nullptr, m_swapChainRTResourceDescHandle[i]);
				m_swapChainRTResource[i] = pBackBuffer; // Store the back buffer resource for later usage
			}
		}
	}

	void Render::_CreateDSV()
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 1;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_dsvDescHeap.GetAddressOf())), "Creating Main DSV Heap Desc failed");

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = DXGI_FORMAT_D32_FLOAT;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		// Create a DSV
		const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		const D3D12_RESOURCE_DESC dsvBufferResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, setting::windowWidth, setting::windowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&dsvBufferResourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optClear,
			IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())), "Creating Main DSV Heap Desc failed");

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void Render::_CreateImGuiSRV()
	{
		// The descriptor heap for the combination of constant-buffer, shader-resource, and unordered-access views.
		D3D12_DESCRIPTOR_HEAP_DESC imguiDesc = {};
		imguiDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		imguiDesc.NumDescriptors = 1;
		imguiDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&imguiDesc, IID_PPV_ARGS(&m_srvImGuiDescHeap)), "Creating ImGui SRV Desc Heap failed");
	}

	void Render::_CreateRootSig()
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf()), "Serializing Root Signature failed");
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf())), "Creating Root Signature failed");
	}

	void Render::_CreatePSO()
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

	#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
		UINT compileFlags = 0;
	#endif
		ThrowIfFailed(D3DCompileFromFile(L"D:\\Programming\\CG\\BlueD-Software-Renderer\\BlueD-Render\\renderer\\src\\render\\shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr), "Compiling vertex shader failed");
		ThrowIfFailed(D3DCompileFromFile(L"D:\\Programming\\CG\\BlueD-Software-Renderer\\BlueD-Render\\renderer\\src\\render\\shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr), "Compiling fragment shader failed");

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
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
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)), "Creating PSO failed");
	}

	void Render::_CreateVertexBuffer()
	{
		// Define the geometry for a triangle.
		Vertex triangleVertices[] =
		{
			{ { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.5f, 0.5f, 1.0f } },
			{ { 0.25f, -0.25f, 0.0f }, { 0.5f, 1.0f, 0.5f, 1.0f } },
			{ { -0.25f, -0.25f, 0.0f }, { 0.5f, 0.5f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.

		const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const D3D12_RESOURCE_DESC vertexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

		// Create the GPU upload buffer.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)), "Creating Vertex Buffer GPU Upload Buffer failed");

		// Copy the triangle data to the vertex buffer (GPU).
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)), "Mapping Vertex Buffer failed");
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	void Render::_CreateSynchronization()
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())), "Creating Fence failed");
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Creating Fence Event failed");
		}
	}

	void Render::_ResetRT()
	{
		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			if (m_swapChainRTResource[i]) { m_swapChainRTResource[i].Reset(); }
	}

	void Render::_WaitForLastSubmittedFrame()
	{
		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
		// sample illustrates how to use fences for efficient resource usage and to
		// maximize GPU utilization.

		// Signal and increment the fence value.
		UINT64 fenceValue = ++m_fenceValue;
		m_commandQueue->Signal(m_fence.Get(), fenceValue);
		m_fenceValue = fenceValue;

		// Wait until the previous frame is finished.
		if (m_fence->GetCompletedValue() < fenceValue)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent), "SetEventOnCompletion failed");
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void Render::ResetCommandAllocator()
	{
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(m_commandAllocator->Reset(), "Resting Command Allocator failed");
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()), "Resting Command List failed");
	}

	void Render::PopulateCommandList()
	{
		// Set necessary state.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		// Draw to viewport
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate that the frame buffer will be used as a render target.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = m_swapChainRTResource[m_frameIndex].Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		m_commandList->ResourceBarrier(1, &barrier);

		// Record commands.
		// Output Merger is the last stage of the pipeline.
		m_commandList->OMSetRenderTargets(1, &m_swapChainRTResourceDescHandle[m_frameIndex], FALSE, nullptr);

		const float clearColor[] = { clear_color.x, clear_color.y, clear_color.z, 1.0f };
		m_commandList->ClearRenderTargetView(m_swapChainRTResourceDescHandle[m_frameIndex], clearColor, 0, nullptr);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

		// Draw
		m_commandList->DrawInstanced(3, 1, 0, 0);

		// ImGui
		ID3D12DescriptorHeap* ppHeaps[] = { m_srvImGuiDescHeap.Get() };
		// Set descriptor heaps (currently only for imgui)
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		// Render ImGui
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

		// Indicate that the back buffer(rt) will now be used to present.
		D3D12_RESOURCE_BARRIER barrier2 = {};
		barrier2.Transition.pResource = m_swapChainRTResource[m_frameIndex].Get();
		barrier2.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier2.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		m_commandList->ResourceBarrier(1, &barrier2);
		
		// Close before execute
		ThrowIfFailed(m_commandList->Close(), "Closing Command List Failed");
	}


	void Render::OnResize(LPARAM lParam)
	{	
		_WaitForLastSubmittedFrame();

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()), "Reseting Command Allocator Failed");

		_ResetRT();

		// Resize swap chain 
		ThrowIfFailed(m_swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH), "Resizing Swap Chain Failed");

		m_frameIndex = 0;

		_CreateRTV();

		// Execute the resize commands.
		ThrowIfFailed(m_commandList->Close(), "Closing Command List Failed");
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Wait until resize is complete.
		_WaitForLastSubmittedFrame();

		// Update the viewport transform to cover the client area.
		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
		m_viewport.Width = static_cast<float>((UINT)LOWORD(lParam));
		m_viewport.Height = static_cast<float>((UINT)HIWORD(lParam));
		m_scissorRect.left = 0;
		m_scissorRect.top = 0;
		m_scissorRect.right = (UINT)LOWORD(lParam);
		m_scissorRect.bottom = (UINT)HIWORD(lParam);

		// TODO:
		// Update the projection matrix.

		// Update the viewport transform to cover the client area.
	}

	void Render::RenderFrame()
	{
		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(nullptr, (void*)m_commandList.Get());
		}

		// Present the frame.
		m_swapChain->Present(1, 0); // Present with vsync

		_WaitForLastSubmittedFrame();
	}

}