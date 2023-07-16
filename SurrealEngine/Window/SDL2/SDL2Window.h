#pragma once

#include "Engine.h"
#include "RenderDevice/RenderDevice.h"
#include <SDL2/SDL.h>

#include "Window/Window.h"

class SDL2Window : public DisplayWindow
{
public:
	SDL2Window(Engine *engine);
	~SDL2Window();
	void OpenWindow(int width, int height, bool fullscreen) override;
	void CloseWindow() override;
	void *GetDisplay() override;
	void *GetWindow() override;
	void Tick() override;
	RenderDevice *GetRenderDevice() override;

private:
	void MouseButton(const SDL_MouseButtonEvent &button, EInputType type);
	void MouseWheel(const SDL_MouseWheelEvent &wheel);
	void MouseMove(const SDL_MouseMotionEvent &motion);
	EInputKey KeyCodeToInputKey(SDL_Keycode keyCode);
	void Key(const SDL_KeyboardEvent &key, EInputType type);

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{
			nullptr, SDL_DestroyWindow};

	std::unique_ptr<RenderDevice> rendDevice;
};
