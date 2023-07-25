#include "SDL3window.h"
#include "Engine.h"
#include "SDL_stdinc.h"
#include "Window/Window.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_syswm.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <exception>
#include <zvulkan/vulkancompatibledevice.h>
#include <zvulkan/vulkansurface.h>

SDL3Window::SDL3Window(Engine *engine)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		throw std::runtime_error("Failed to initialize SDL: " +
														 std::string(SDL_GetError()));

	if (SDL_Vulkan_LoadLibrary(nullptr) != 0)
		throw std::runtime_error("Failed to load Vulkan library: " +
														 std::string(SDL_GetError()));
}

SDL3Window::~SDL3Window()
{
	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
}

void SDL3Window::OpenWindow(int width, int height, bool fullscreen)
{
	CloseWindow();
	SizeX = width;
	SizeY = height;
	CreateSdlWindow(fullscreen);
	SetWindowOptions();
	auto vulkanBuilder = CreateVulkanInstanceBuilder();
	auto instance{vulkanBuilder.DebugLayer(false).Create()};
	VkSurfaceKHR surfaceHandle{};

	if (!SDL_Vulkan_CreateSurface(sdlWindow.get(), instance->Instance,
																&surfaceHandle))
		throw std::runtime_error("Failed to create Vulkan surface: " +
														 std::string(SDL_GetError()));

	auto surface{std::make_shared<VulkanSurface>(instance, surfaceHandle)};
	rendDevice = RenderDevice::Create(this, surface);
}

void SDL3Window::CloseWindow()
{
	rendDevice.reset();
	sdlWindow.reset();
}

void *SDL3Window::GetDisplay()
{

	return reinterpret_cast<void *>(SDL_GetDisplayForWindow(sdlWindow.get()));
}

void *SDL3Window::GetWindow() { return static_cast<void *>(sdlWindow.get()); }

void SDL3Window::Tick()
{

	SDL_Event event{};
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			engine->WindowClose(this);
			continue;

		case SDL_EVENT_WINDOW_RESIZED:
			SizeX = event.window.data1;
			SizeY = event.window.data2;

			continue;

		case SDL_EVENT_MOUSE_MOTION:
			MouseMove(event.motion);
			continue;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			MouseButton(event.button, IST_Press);
			continue;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			MouseButton(event.button, IST_Release);
			continue;

		case SDL_EVENT_MOUSE_WHEEL:
			MouseWheel(event.wheel);
			continue;

		case SDL_EVENT_KEY_DOWN:
			Key(event.key, IST_Press);
			continue;

		case SDL_EVENT_KEY_UP:
			Key(event.key, IST_Release);
			continue;

		default:
			break;
		}
	}
}

RenderDevice *SDL3Window::GetRenderDevice() { return rendDevice.get(); }

void SDL3Window::CreateSdlWindow(bool fullscreen)
{
	Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
	sdlWindow.reset(SDL_CreateWindow("SurrealEngine", SizeX, SizeY, flags));

	if (!sdlWindow)
		throw std::runtime_error("Failed to create SDL window: " +
														 std::string(SDL_GetError()));

	if (fullscreen)
	{
		this->isFullscreen = true;
		SDL_SetWindowFullscreen(sdlWindow.get(), SDL_bool::SDL_TRUE);
	}
}

void SDL3Window::SetWindowOptions()
{
	// SDL_ShowCursor(SDL_DISABLE);
	SDL_HideCursor();
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

VulkanInstanceBuilder SDL3Window::CreateVulkanInstanceBuilder()
{

	unsigned int extensionCount{0};
	if (!SDL_Vulkan_GetInstanceExtensions(&extensionCount, nullptr))
		throw std::runtime_error(
				"Failed to get the number of required Vulkan extensions from SDL: " +
				std::string(SDL_GetError()));

	std::vector<const char *> extensions(extensionCount);

	if (!SDL_Vulkan_GetInstanceExtensions(&extensionCount, extensions.data()))
		throw std::runtime_error(
				"Failed to get the required Vulkan extensions from SDL: " +
				std::string(SDL_GetError()));

	auto vulkanBuilder = VulkanInstanceBuilder();

	for (const char *extension : extensions)
		vulkanBuilder.RequireExtension(extension);

	return vulkanBuilder;
}

void SDL3Window::MouseMove(const SDL_MouseMotionEvent &motion)
{
	engine->InputEvent(this, IK_MouseX, IST_Axis, motion.xrel);
	engine->InputEvent(this, IK_MouseY, IST_Axis, -motion.yrel);
}

void SDL3Window::MouseButton(const SDL_MouseButtonEvent &button,
														 EInputType type)
{
	switch (button.button)
	{
	case SDL_BUTTON_LEFT:
		engine->InputEvent(this, IK_LeftMouse, type);
		break;

	case SDL_BUTTON_RIGHT:
		engine->InputEvent(this, IK_RightMouse, type);
		break;

	case SDL_BUTTON_MIDDLE:
		engine->InputEvent(this, IK_MiddleMouse, type);
		break;

	default:
		break;
	};
}

void SDL3Window::MouseWheel(const SDL_MouseWheelEvent &wheel)
{
	if (wheel.y > 0)
		engine->InputEvent(this, IK_MouseWheelDown, IST_Press);
	else if (wheel.y < 0)
		engine->InputEvent(this, IK_MouseWheelUp, IST_Press);
}

EInputKey SDL3Window::KeyCodeToInputKey(SDL_Keycode keyCode)
{
	switch (keyCode)
	{
	case SDLK_BACKSPACE:
		return IK_Backspace;
	case SDLK_TAB:
		return IK_Tab;
	case SDLK_CLEAR:
		return IK_OEMClear;
	case SDLK_RETURN:
		return IK_Enter;
	case SDLK_ALTERASE:
		return IK_Alt;
	case SDLK_PAUSE:
		return IK_Pause;
	case SDLK_ESCAPE:
		return IK_Escape;
	case SDLK_SPACE:
		return IK_Space;
	case SDLK_END:
		return IK_End;
	case SDLK_HOME:
		return IK_Home;
	case SDLK_LEFT:
		return IK_Left;
	case SDLK_UP:
		return IK_Up;
	case SDLK_RIGHT:
		return IK_Right;
	case SDLK_DOWN:
		return IK_Down;
	case SDLK_SELECT:
		return IK_Select;
	case SDLK_PRINTSCREEN:
		return IK_Print;
	case SDLK_EXECUTE:
		return IK_Execute;
	case SDLK_INSERT:
		return IK_Insert;
	case SDLK_DELETE:
		return IK_Delete;
	case SDLK_HELP:
		return IK_Help;
	case SDLK_0:
		return IK_0;
	case SDLK_1:
		return IK_1;
	case SDLK_2:
		return IK_2;
	case SDLK_3:
		return IK_3;
	case SDLK_4:
		return IK_4;
	case SDLK_5:
		return IK_5;
	case SDLK_6:
		return IK_6;
	case SDLK_7:
		return IK_7;
	case SDLK_8:
		return IK_8;
	case SDLK_9:
		return IK_9;
	case SDLK_a:
		return IK_A;
	case SDLK_b:
		return IK_B;
	case SDLK_c:
		return IK_C;
	case SDLK_d:
		return IK_D;
	case SDLK_e:
		return IK_E;
	case SDLK_f:
		return IK_F;
	case SDLK_g:
		return IK_G;
	case SDLK_h:
		return IK_H;
	case SDLK_i:
		return IK_I;
	case SDLK_j:
		return IK_J;
	case SDLK_k:
		return IK_K;
	case SDLK_l:
		return IK_L;
	case SDLK_m:
		return IK_M;
	case SDLK_n:
		return IK_N;
	case SDLK_o:
		return IK_O;
	case SDLK_p:
		return IK_P;
	case SDLK_q:
		return IK_Q;
	case SDLK_r:
		return IK_R;
	case SDLK_s:
		return IK_S;
	case SDLK_t:
		return IK_T;
	case SDLK_u:
		return IK_U;
	case SDLK_v:
		return IK_V;
	case SDLK_w:
		return IK_W;
	case SDLK_x:
		return IK_X;
	case SDLK_y:
		return IK_Y;
	case SDLK_z:
		return IK_Z;
	case SDLK_KP_0:
		return IK_NumPad0;
	case SDLK_KP_1:
		return IK_NumPad1;
	case SDLK_KP_2:
		return IK_NumPad2;
	case SDLK_KP_3:
		return IK_NumPad3;
	case SDLK_KP_4:
		return IK_NumPad4;
	case SDLK_KP_5:
		return IK_NumPad5;
	case SDLK_KP_6:
		return IK_NumPad6;
	case SDLK_KP_7:
		return IK_NumPad7;
	case SDLK_KP_8:
		return IK_NumPad8;
	case SDLK_KP_9:
		return IK_NumPad9;
	case SDLK_SEPARATOR:
		return IK_Separator;
	case SDLK_KP_DECIMAL:
		return IK_NumPadPeriod;
	case SDLK_F1:
		return IK_F1;
	case SDLK_F2:
		return IK_F2;
	case SDLK_F3:
		return IK_F3;
	case SDLK_F4:
		return IK_F4;
	case SDLK_F5:
		return IK_F5;
	case SDLK_F6:
		return IK_F6;
	case SDLK_F7:
		return IK_F7;
	case SDLK_F8:
		return IK_F8;
	case SDLK_F9:
		return IK_F9;
	case SDLK_F10:
		return IK_F10;
	case SDLK_F11:
		return IK_F11;
	case SDLK_F12:
		return IK_F12;
	case SDLK_F13:
		return IK_F13;
	case SDLK_F14:
		return IK_F14;
	case SDLK_F15:
		return IK_F15;
	case SDLK_F16:
		return IK_F16;
	case SDLK_F17:
		return IK_F17;
	case SDLK_F18:
		return IK_F18;
	case SDLK_F19:
		return IK_F19;
	case SDLK_F20:
		return IK_F20;
	case SDLK_F21:
		return IK_F21;
	case SDLK_F22:
		return IK_F22;
	case SDLK_F23:
		return IK_F23;
	case SDLK_F24:
		return IK_F24;
	case SDLK_NUMLOCKCLEAR:
		return IK_NumLock;
	case SDLK_SCROLLLOCK:
		return IK_ScrollLock;
	case SDLK_LSHIFT:
		return IK_LShift;
	case SDLK_RSHIFT:
		return IK_RShift;
	case SDLK_LCTRL:
		return IK_LControl;
	case SDLK_RCTRL:
		return IK_RControl;
	case SDLK_BACKQUOTE:
		return IK_Tilde;
	default:
		return IK_None;
	}
}

void SDL3Window::Key(const SDL_KeyboardEvent &key, EInputType type)
{
	if (type == IST_Press && key.keysym.sym == SDLK_RETURN &&
			key.keysym.mod & SDL_KMOD_ALT)
	{
		isFullscreen = !isFullscreen;
		SDL_SetWindowFullscreen(sdlWindow.get(), isFullscreen ? SDL_TRUE : SDL_FALSE);
	}

	auto inputKey{KeyCodeToInputKey(key.keysym.sym)};

	if (inputKey == IK_None)
		return;

	engine->InputEvent(this, inputKey, type);

	if (type == IST_Press && key.keysym.sym >= 0 && key.keysym.sym <= 127)
	{
		const std::string keyStr{static_cast<char>(key.keysym.sym)};
		engine->Key(this, keyStr);
	}
}
