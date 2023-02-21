#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <windows.h> 

#include "d3dx12.h"
#include "dx_helper.h"

class DXBlue
{
public:
	uint32_t											g_ScreenWidth;
	uint32_t											g_ScreenHeight;
	HWND												g_hWnd;
	static const uint32_t								g_FrameCount = 3;
	Microsoft::WRL::ComPtr<IDXGIFactory4> 				g_pDXGIFactory;
	Microsoft::WRL::ComPtr<ID3D12Device>				g_pDevice;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS		g_msQualityLevels;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	        g_pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		g_pCommandAllocator[g_FrameCount];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	g_pCommandList;		// Send to GPU
	Microsoft::WRL::ComPtr<IDXGISwapChain3>				g_pSwapChain;
	HANDLE												g_pSwapChainWaitableObject;
	uint32_t											g_pFrameIndex; // current rtv we are on

	Microsoft::WRL::ComPtr<ID3D12Fence>					g_pFence[g_FrameCount];
	uint64_t											g_pFenceValue[g_FrameCount];
	HANDLE												g_pFenceEvent; // a handle to an event when our fence is unlocked by the gpu
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		g_pRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		g_pDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		g_pSrvHeap;
	uint32_t											g_rtvDescriptorSize;
	uint32_t											g_currentBackBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>				g_SwapChainBuffer[g_FrameCount];
	Microsoft::WRL::ComPtr<ID3D12Resource>				g_DepthStencilBuffer;

	D3D12_VIEWPORT										g_ScreenViewport;
	D3D12_RECT											g_ScissorRect;

	void Init();
	
	void CreateDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	// Buffers are abstracted as resources
	void CreateDescriptorHeaps();
	void CreateBuffers();

	void UpdatePipeline();
	void WaitForPreviousFrame();
	uint32_t WaitForNextFrameResources();

	void Render();
	void Cleanup();

	DXBlue(uint32_t width, uint32_t height, HWND hwnd);
	~DXBlue();
};
