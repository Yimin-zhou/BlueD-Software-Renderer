#pragma once

#include <string>

#include "d3dx12.h"


inline void ThrowIfFailed(HRESULT hr, const std::string& msg)
{
	if (FAILED(hr))
	{
		// Set a breakpoint on this line to catch DirectX API errors
		throw std::exception(msg.c_str());
	}
}
