#pragma once

#include "imgui.h"

inline void AddGui()
{
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	ImGui::ShowDemoWindow();

	// 2. Show Debug window
	{
		ImGui::Begin("Debug: ");

		ImGui::Text("This is some useful text.");

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}
}