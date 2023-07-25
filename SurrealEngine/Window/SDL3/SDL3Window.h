#pragma once

#include "Engine.h"
#include "RenderDevice/RenderDevice.h"
#include "Window/Window.h"
#include <SDL3/SDL.h>
#include <zvulkan/vulkanbuilders.h>

class SDL3Window : public DisplayWindow
{
public:
	SDL3Window(Engine *engine);
	~SDL3Window();
	void OpenWindow(int width, int height, bool fullscreen) override;
	void CloseWindow() override;
	void *GetDisplay() override;
	void *GetWindow() override;
	void Tick() override;
	RenderDevice *GetRenderDevice() override;

private:
	void CreateSdlWindow(bool fullscreen);
	void SetWindowOptions();
	VulkanInstanceBuilder CreateVulkanInstanceBuilder();
	void MouseMove(const SDL_MouseMotionEvent &motion);
	void MouseButton(const SDL_MouseButtonEvent &button, EInputType type);
	void MouseWheel(const SDL_MouseWheelEvent &wheel);
	EInputKey KeyCodeToInputKey(SDL_Keycode keyCode);
	void Key(const SDL_KeyboardEvent &key, EInputType type);

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{
			nullptr, SDL_DestroyWindow};

	bool isFullscreen{false};
	std::unique_ptr<RenderDevice> rendDevice;
};
