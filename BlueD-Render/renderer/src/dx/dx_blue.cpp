#include "dx_blue.h"

using Microsoft::WRL::ComPtr;

DXBlue::DXBlue(uint32_t width, uint32_t height, HWND hwnd) : g_ScreenWidth(width), g_ScreenHeight(height), g_hWnd(hwnd){};

DXBlue::~DXBlue(){}

void DXBlue::Init()
{
	CreateDevice();
	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeaps();
	CreateBuffers();
}

void DXBlue::CreateDevice()
{

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&g_pDXGIFactory)));

	// Find first hardware gpu that supports d3d 12
	ComPtr<IDXGIAdapter> pWarpAdapter;
	// Enumerate adapters
	ThrowIfFailed(g_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
	// create deivce
	ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_pDevice)));

	// msaa
	g_msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	g_msQualityLevels.SampleCount = 4;
	g_msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	g_msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(g_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &g_msQualityLevels, sizeof(g_msQualityLevels)));
}

void DXBlue::CreateCommandObjects()
{
	// Describe a command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // Direct means the gpu can directly execute this command queue
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(g_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pCommandQueue)));

	// For allocating command buffer
	for (int i = 0; i < g_FrameCount; i++)
	{
		ThrowIfFailed(g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCommandAllocator[i])));
	}

	// Create command list
	// create the command list with the first allocator
	ThrowIfFailed(g_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pCommandAllocator->Get(), nullptr, IID_PPV_ARGS(&g_pCommandList)));

	// command lists are created in the recording state. our main loop will set it up for recording again so close it now
	g_pCommandList->Close();
}

void DXBlue::CreateSwapChain()
{
	DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
	backBufferDesc.Width = g_ScreenWidth; // buffer width
	backBufferDesc.Height = g_ScreenHeight; // buffer height
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the buffer (rgba 32 bits, 8 bits for each chanel)

	// describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = g_FrameCount; // number of buffers we have
	swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
	swapChainDesc.OutputWindow = g_hWnd; // handle to our window
	swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present

	IDXGISwapChain* tempSwapChain;
	ThrowIfFailed(g_pDXGIFactory->CreateSwapChain(g_pCommandQueue.Get(), &swapChainDesc, &tempSwapChain));
	g_pSwapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
	g_pFrameIndex = g_pSwapChain->GetCurrentBackBufferIndex();
	g_pSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
}

void DXBlue::CreateDescriptorHeaps()
{
	// Control CPU and GPU sync
	// Create the fences
	for (int i = 0; i < g_FrameCount; i++)
	{
		ThrowIfFailed(g_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence[i])));
		g_pFenceValue[i] = 0; // set the initial fence value to 0
	}
	// create a handle to a fence event
	g_pFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = g_FrameCount; 
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// This heap will not be directly referenced by the shaders (not shader visible), as this will store the output from the pipeline
	// otherwise we would set the heap's flag to D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(g_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_pRtvHeap)));
	// get the size of a descriptor in this heap (this is a rtv heap, so only rtv descriptors should be stored in it.
	// descriptor sizes may vary from device to device, which is why there is no set size and we must ask the 
	// device to give us the size. we will use this size to increment a descriptor handle offset
	g_rtvDescriptorSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Describe and create a  depth - stencil View descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(g_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_pDsvHeap)));

	// The descriptor heap for the combination of constant-buffer, shader-resource, and unordered-access views.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(g_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g_pSrvHeap)));
}

void DXBlue::CreateBuffers()
{
	// Release buffers
	for (int i = 0; i < g_FrameCount; ++i) g_SwapChainBuffer[i].Reset();
	g_DepthStencilBuffer.Reset();

	// Reallocate buffer according to window size
	ThrowIfFailed(g_pSwapChain->ResizeBuffers(g_FrameCount,g_ScreenWidth, g_ScreenHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	g_currentBackBuffer = 0;

	// Create RTV
	// get a handle to the first descriptor in the descriptor heap.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
	// Create a RTV for each buffer.
	for (int i = 0; i < g_FrameCount; ++i) 
	{
		ThrowIfFailed(g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&g_SwapChainBuffer[i])));
		// we "create" a render target view which binds the swap chain buffer (Render targets) (ID3D12Resource[n]) to the rtv handle
		g_pDevice->CreateRenderTargetView(g_SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// Next desc
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}

	// Create DSV
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = g_ScreenWidth;
	depthStencilDesc.Height = g_ScreenHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES tempHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(g_pDevice->CreateCommittedResource(
		&tempHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(&g_DepthStencilBuffer)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	g_pDevice->CreateDepthStencilView(g_DepthStencilBuffer.Get(), &dsvDesc, g_pDsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Transition to depth write
	CD3DX12_RESOURCE_BARRIER tempResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		g_DepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	g_pCommandList->ResourceBarrier(
		1,
		&tempResourceBarrier);
}

void DXBlue::UpdatePipeline()
{
	// We have to wait for the gpu to finish with the command allocator before we reset it
	WaitForPreviousFrame();

	// we can only reset an allocator once the gpu is done with it
	// resetting an allocator frees the memory that the command list was stored in
	ThrowIfFailed(g_pCommandAllocator[g_pFrameIndex]->Reset());

	// reset the command list. by resetting the command list we are putting it into
	// a recording state so we can start recording commands into the command allocator.
	// the command allocator that we reference here may have multiple command lists
	// associated with it, but only one can be recording at any time. Make sure
	// that any other command lists associated to this command allocator are in
	// the closed state (not recording).
	// Here you will pass an initial pipeline state object as the second parameter,
	// but in this tutorial we are only clearing the rtv, and do not actually need
	// anything but an initial default pipeline, which is what we get by setting
	// the second parameter to NULL
	ThrowIfFailed(g_pCommandList->Reset(g_pCommandAllocator[g_pFrameIndex].Get(), nullptr));

	// here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)
	// transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	CD3DX12_RESOURCE_BARRIER tempResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(g_SwapChainBuffer[g_pFrameIndex].Get(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_pCommandList->ResourceBarrier(1, &tempResourceBarrier);

	// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), g_pFrameIndex, g_rtvDescriptorSize);

	// set the render target for the output merger stage (the output of the pipeline)
	g_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear the render target by using the ClearRenderTargetView command
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	g_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
	// warning if present is called on the render target when it's not in the present state
	CD3DX12_RESOURCE_BARRIER tempResourceBarrier_2 = CD3DX12_RESOURCE_BARRIER::Transition(g_SwapChainBuffer[g_pFrameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	g_pCommandList->ResourceBarrier(1, &tempResourceBarrier_2);

	ThrowIfFailed(g_pCommandList->Close());
}

void DXBlue::Render()
{
	UpdatePipeline(); // update the pipeline by sending commands to the commandqueue

	// create an array of command lists (only one command list here)
	ID3D12CommandList* ppCommandLists[] = { g_pCommandList.Get() };

	// execute the array of command lists
	g_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// this command goes in at the end of our command queue. we will know when our command queue 
	// has finished because the fence value will be set to "fenceValue" from the GPU since the command
	// queue is being executed on the GPU
	ThrowIfFailed(g_pCommandQueue->Signal(g_pFence[g_pFrameIndex].Get(), g_pFenceValue[g_pFrameIndex]));
	// present the current backbuffer
	ThrowIfFailed(g_pSwapChain->Present(1, 0));

}

void DXBlue::Cleanup()
{
	// wait for the gpu to finish all frames
	for (int i = 0; i < g_FrameCount; ++i)
	{
		g_pFrameIndex = i;
		WaitForPreviousFrame();
	}

	SAFE_RELEASE(g_pDevice);
	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pCommandQueue);
	SAFE_RELEASE(g_pRtvHeap);
	SAFE_RELEASE(g_pDsvHeap);
	SAFE_RELEASE(g_pSrvHeap);
	SAFE_RELEASE(g_pCommandList);

	for (int i = 0; i < g_FrameCount; ++i)
	{
		SAFE_RELEASE(g_SwapChainBuffer[i]);
		SAFE_RELEASE(g_pCommandAllocator[i]);
		SAFE_RELEASE(g_pFence[i]);
	};
}

void DXBlue::WaitForPreviousFrame()
{
	// if the current fence value is still less than "g_pFenceValue", then we know the GPU has not finished executing
	// the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
	if (g_pFence[g_pFrameIndex]->GetCompletedValue() < g_pFenceValue[g_pFrameIndex])
	{
		// we have the fence create an event which is signaled once the fence's current value is "g_pFenceValue"
		ThrowIfFailed(g_pFence[g_pFrameIndex]->SetEventOnCompletion(g_pFenceValue[g_pFrameIndex], g_pFenceEvent));

		// We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
		// has reached "fenceValue", we know the command queue has finished executing
		WaitForSingleObject(g_pFenceEvent, INFINITE);
	}

	// increment fenceValue for next frame
	g_pFenceValue[g_pFrameIndex]++;

	// swap the current rtv buffer index so we draw on the correct buffer
	g_pFrameIndex = g_pSwapChain->GetCurrentBackBufferIndex();
}

uint32_t DXBlue::WaitForNextFrameResources()
{
	uint32_t nextFrameIndex = g_pFrameIndex + 1;
	g_pFrameIndex = nextFrameIndex;

	HANDLE waitableObjects[] = { g_pSwapChainWaitableObject, nullptr };
	DWORD numWaitableObjects = 1;
	uint32_t index = nextFrameIndex % g_FrameCount;

	ID3D12CommandAllocator* tempCommandAllocator = g_pCommandAllocator[index].Get();
	uint64_t fenceValue = g_pFenceValue[index];

	if (fenceValue != 0) // means no fence was signaled
	{
		g_pFenceValue[index] = 0;
		g_pFence[index]->SetEventOnCompletion(fenceValue, g_pFenceEvent);
		waitableObjects[1] = g_pFenceEvent;
		numWaitableObjects = 2;
	}

	WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

	return nextFrameIndex;
}