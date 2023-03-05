#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <cstdint>
#include <utility>
#include <wrl.h>
#include <DirectXMath.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

namespace blue
{
	//using namespace Microsoft::WRL;
	struct FrameContext
	{
		ID3D12CommandAllocator* CommandAllocator;
		int64_t                 FenceValue;
	};

	using namespace Microsoft::WRL;
	class Render
	{
	public:
		ImVec4		clear_color;
		static const int32_t                NUM_FRAMES_IN_FLIGHT = 3;
		static const int32_t                NUM_BACK_BUFFERS = 3;
		FrameContext                g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
		uint32_t                     g_frameIndex;

		// use com ptr
		ComPtr<ID3D12Device>				g_pd3dDevice;
		ComPtr<ID3D12DescriptorHeap>		g_pd3dRtvDescHeap;
		ComPtr<ID3D12DescriptorHeap>		g_pd3dSrvDescHeap;
		ComPtr<ID3D12CommandQueue>			g_pd3dCommandQueue;
		ComPtr<ID3D12GraphicsCommandList>	g_pd3dCommandList;
		ComPtr<ID3D12Fence>					g_fence;
		HANDLE								g_fenceEvent;
		uint64_t							g_fenceLastSignaledValue;
		ComPtr<IDXGISwapChain3>					g_pSwapChain;
		HANDLE								g_hSwapChainWaitableObject;
		ComPtr<ID3D12Resource>				g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

		HWND					windowHandler;

		bool CreateDevice();
		void DestroyDevice();

		FrameContext* WaitForNextFrameResources();

		void CreateRenderTarget();
		void CleanupRenderTarget();
		void WaitForLastSubmittedFrame();

		void RenderFrame();

		Render(HWND windowHandler);
		Render();
		~Render();

	private:

	};
}