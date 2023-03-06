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
		static const int32_t                NUM_FRAMES_IN_FLIGHT = 2;
		static const int32_t                NUM_BACK_BUFFERS = 2;

		CD3DX12_VIEWPORT 					m_viewport;
		CD3DX12_RECT 						m_scissorRect;

		ComPtr<IDXGISwapChain3>				m_swapChain;
		ComPtr<ID3D12Device>				m_device;
		ComPtr<IDXGIFactory4>				m_dxgiFactory4;
		ComPtr<IDXGIAdapter1>				m_adapter;

		ComPtr<ID3D12Resource>				m_depthStencilBuffer;
		ComPtr<ID3D12Resource>				m_swapChainRTResource[NUM_BACK_BUFFERS]; // swap chain render target
		D3D12_CPU_DESCRIPTOR_HANDLE			m_swapChainRTResourceDescHandle[NUM_BACK_BUFFERS]; 

		ComPtr<ID3D12GraphicsCommandList>	m_commandList;
		ComPtr<ID3D12CommandQueue>			m_commandQueue;
		ComPtr<ID3D12CommandAllocator> 		m_commandAllocator;

		ComPtr<ID3D12DescriptorHeap>		m_swapChainRtvDescHeap;
		ComPtr<ID3D12DescriptorHeap>		m_dsvDescHeap;
		ComPtr<ID3D12DescriptorHeap>		m_srvImGuiDescHeap;
		


		ComPtr<ID3D12RootSignature>			m_rootSignature;
		ComPtr<ID3D12PipelineState>			m_pipelineState;
		HANDLE								m_swapChainWaitableObject;

		// resources
		ComPtr<ID3D12Resource>				m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW			m_vertexBufferView;

		// Synchronization objects.
		uint32_t							m_frameIndex;
		HANDLE								m_fenceEvent;
		ComPtr<ID3D12Fence>					m_fence;
		uint64_t							m_fenceValue;


		ImVec4								clear_color;
		HWND								windowHandler;

		void Init();
		void Cleanup();

		void LoadPipeline();
		void LoadAsset();

		void ResetCommandAllocator();
		void PopulateCommandList();
		void RenderFrame();
		void OnResize(LPARAM lParam);

		Render(HWND windowHandler);
		Render();
		~Render();

	private:
		// Pipeline
		void _CreateDevice();
		void _CreateCommandQueue();
		void _CreateSwapchain();
		void _CreateRTV();
		void _CreateDSV();
		void _CreateImGuiSRV();
		void _CreateCmdAllocator();

		// Assets
		void _CreateRootSig();
		void _CreatePSO();
		void _CreateCommandList();
		void _CreateVertexBuffer();
		void _CreateSynchronization();

		void _ResetRT();
		void _WaitForLastSubmittedFrame();

	};
}