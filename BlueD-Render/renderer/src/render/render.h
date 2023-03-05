#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <cstdint>
#include <utility>
#include <wrl.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"
#include "helper.h"

using Microsoft::WRL::ComPtr;

namespace blue
{
	struct FrameContext
	{
		ComPtr<ID3D12CommandAllocator>		CommandAllocator;
		int64_t								FenceValue;
	};

	class Render
	{
	public:
		// define a vertex struct
		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT4 color;
		};

		// Pipeline objects.
		static const int32_t                NUM_FRAMES_IN_FLIGHT = 3;
		static const int32_t                NUM_BACK_BUFFERS = 3;
		CD3DX12_VIEWPORT 					g_viewport;
		CD3DX12_RECT 						g_scissorRect;
		ComPtr<IDXGISwapChain3>				g_pSwapChain;
		ComPtr<ID3D12Device>				g_pd3dDevice;
		ComPtr<IDXGIAdapter1>				g_pAdapter;
		ComPtr<ID3D12Resource>				g_mainRenderTargetResource[NUM_BACK_BUFFERS];
		D3D12_CPU_DESCRIPTOR_HANDLE			g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS];
		FrameContext						g_frameContext[NUM_FRAMES_IN_FLIGHT];
		ComPtr<ID3D12CommandQueue>			g_pd3dCommandQueue;
		ComPtr<ID3D12RootSignature>			g_pd3dRootSignature;
		ComPtr<ID3D12DescriptorHeap>		g_pd3dRtvDescHeap;
		ComPtr<ID3D12DescriptorHeap>		g_pd3dSrvDescHeap;
		ComPtr<ID3D12GraphicsCommandList>	g_pd3dCommandList;
		ComPtr<ID3D12PipelineState>			g_pd3dPipelineState;
		HANDLE								g_hSwapChainWaitableObject;

		// resources
		ComPtr<ID3D12Resource>				g_pd3dVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW			g_pd3dVertexBufferView;

		// Synchronization objects.
		uint32_t							g_frameIndex;
		HANDLE								g_fenceEvent;
		ComPtr<ID3D12Fence>					g_fence;
		uint64_t							g_fenceLastSignaledValue;


		ImVec4								clear_color;
		HWND								windowHandler;

		void CreateDevice();
		void DestroyDevice();

		FrameContext* WaitForNextFrameResources();

		void CreateRenderTarget();
		void CleanupRenderTarget();
		void WaitForLastSubmittedFrame();

		void PopulateCommandList();
		void RenderFrame();

		Render(HWND windowHandler);
		Render();
		~Render();

	private:

	};
}