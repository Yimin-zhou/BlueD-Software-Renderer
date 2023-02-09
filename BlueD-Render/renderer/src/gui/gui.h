#pragma once

#include <windows.h> 
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>

namespace blueGUI
{
	// Windows window
	extern bool quit;
	constexpr int windowWidth = 800;
	constexpr int windowHeight = 500;
	// winapi window variables
	extern HWND window;
	extern WNDCLASSEXW	windowClass;
	// points for window movement
	extern POINTS position;
	extern ImVec4 clear_color;

	// dx12
	//extern ImGuiIO& io;
	extern struct FrameContext;

	// Data
	extern int const                    NUM_FRAMES_IN_FLIGHT;
	extern UINT                         g_frameIndex;

	extern int const                    NUM_BACK_BUFFERS ;
	extern ID3D12Device*				g_pd3dDevice;
	extern ID3D12DescriptorHeap*		g_pd3dRtvDescHeap;
	extern ID3D12DescriptorHeap*		g_pd3dSrvDescHeap;
	extern ID3D12CommandQueue*			g_pd3dCommandQueue;
	extern ID3D12GraphicsCommandList*	g_pd3dCommandList;
	extern ID3D12Fence*					g_fence;
	extern HANDLE                       g_fenceEvent;
	extern UINT64                       g_fenceLastSignaledValue;
	extern IDXGISwapChain3*				g_pSwapChain;
	extern HANDLE                       g_hSwapChainWaitableObject;

	void CreateHWindow();
	void DestroyHWindow();
	bool CreateDevice();
	void DestroyDevice();

	ImGuiIO CreateGui();
	void DestroyGui();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	void WaitForLastSubmittedFrame();

	void StartRenderGui();
	void RenderGui(ImGuiIO& io);

}