#pragma once

#include <windows.h> 
#include <d3d12.h>
#include <dxgi1_4.h>
#include <cstdint>
#include <wrl/client.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

namespace blueGUI
{
	// Windows window
	extern bool			quit;
	constexpr uint32_t	windowWidth = 800;
	constexpr uint32_t	windowHeight = 500;
	// winapi window variables
	extern HWND			window;
	extern WNDCLASSEXW	windowClass;
	// points for window movement
	extern POINTS		position;
	extern ImVec4		clear_color;

	// DX12
	extern struct FrameContext;
	// Data
	extern int32_t const                NUM_FRAMES_IN_FLIGHT;
	extern uint32_t                     g_frameIndex;
	extern int32_t const                NUM_BACK_BUFFERS ;
	extern Microsoft::WRL::ComPtr<ID3D12Device>				g_pd3dDevice;
	extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		g_pd3dRtvDescHeap;
	extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		g_pd3dSrvDescHeap;
	extern Microsoft::WRL::ComPtr<ID3D12CommandQueue>			g_pd3dCommandQueue;
	extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	g_pd3dCommandList;
	extern Microsoft::WRL::ComPtr<IDXGISwapChain3>				g_pSwapChain;
	extern Microsoft::WRL::ComPtr<ID3D12Fence>					g_fence;
	extern HANDLE                       g_fenceEvent;
	extern uint64_t                     g_fenceLastSignaledValue;
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