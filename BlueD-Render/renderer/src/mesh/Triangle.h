#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>

namespace blue
{
	// define a triangle class
	class Triangle
	{
		public:
		// define a vertex struct
		struct Vertex
		{
			DirectX::XMFLOAT4 position;
			DirectX::XMFLOAT4 color;
		};

		// define a triangle
		Triangle();
		~Triangle();

		// create a triangle
		void CreateTriangle();

		// destroy a triangle
		void DestroyTriangle();

		// get the vertex buffer
		ID3D12Resource* GetVertexBuffer();

		// get the vertex buffer view
		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

		// get the vertex buffer size
		uint32_t GetVertexBufferViewSize();

		// get the vertex buffer stride
		uint32_t GetVertexBufferViewStride();
	};
}
