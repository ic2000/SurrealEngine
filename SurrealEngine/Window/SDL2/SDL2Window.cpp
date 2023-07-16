#define VK_USE_PLATFORM_XLIB_KHR

#include "SDL2Window.h"
#include "Engine.h"
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <cassert>
#include <iostream>
#include <zvulkan/vulkanbuilders.h>
#include <zvulkan/vulkancompatibledevice.h>
#include <zvulkan/vulkansurface.h>

SDL2Window::SDL2Window(Engine *engine)
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	assert(SDL_Vulkan_LoadLibrary(nullptr) == 0);
}

SDL2Window::~SDL2Window()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
}

void SDL2Window::OpenWindow(int width, int height, bool fullscreen)
{
	SizeX = width;
	SizeY = height;

	std::cout << __PRETTY_FUNCTION__ << std::endl;

	Uint32 flags = SDL_WINDOW_VULKAN;
	// if (fullscreen)
	// flags |= SDL_WINDOW_FULLSCREEN;

	sdlWindow.reset(SDL_CreateWindow("UTEngine", SDL_WINDOWPOS_UNDEFINED,
																	 SDL_WINDOWPOS_UNDEFINED, width, height,
																	 flags));

	assert(sdlWindow);

	unsigned int extensionCount = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(sdlWindow.get(), &extensionCount,
																				nullptr))
	{
		// Handle extension retrieval error

		std::cout << "Failed to get the required Vulkan extensions from SDL: "
							<< SDL_GetError() << std::endl;
	}

	std::vector<const char *> extensions(extensionCount);
	if (!SDL_Vulkan_GetInstanceExtensions(sdlWindow.get(), &extensionCount,
																				extensions.data()))
	{
		// Handle extension retrieval error

		std::cout << "Failed to get the required Vulkan extensions from SDL: "
							<< SDL_GetError() << std::endl;
	}

	auto builder = VulkanInstanceBuilder();

	for (const char *extension : extensions)
		builder.RequireExtension(extension);

	auto instance{builder.DebugLayer(true).Create()};

	// auto instance = VulkanInstanceBuilder()
	// 										.RequireExtension(VK_KHR_XLIB_SURFACE_EXTENSION_NAME)
	// 										.RequireSurfaceExtensions()
	// 										.DebugLayer(true)
	// 										.Create();

	VkSurfaceKHR vk_surface{};

	if (!SDL_Vulkan_CreateSurface(sdlWindow.get(), instance->Instance,
																&vk_surface))
	{
		// Handle surface creation error
		std::cout << "Failed to create Vulkan surface: " << SDL_GetError()
							<< std::endl;

		return;
	}

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance->Instance, &deviceCount, nullptr);

	if (deviceCount > 0)
	{
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance->Instance, &deviceCount,
															 devices.data());

		for (const auto &device : devices)
		{
			VkBool32 surfaceSupported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, 0, vk_surface,
																					 &surfaceSupported);

			if (surfaceSupported == VK_TRUE)
			{
				physicalDevice = device;
				break;
			}
		}
	}
	else
	{
		std::cout << "No Vulkan-compatible physical devices found." << std::endl;
		return;
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		std::cout << "Surface is not supported by any physical device."
							<< std::endl;
		return;
	}
	else
	{
		std::cout << "Surface is supported by the selected physical device."
							<< std::endl;
	}

	auto surface = std::make_shared<VulkanSurface>(instance, vk_surface);
	rendDevice = RenderDevice::Create(this, surface);
}

void SDL2Window::CloseWindow()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	// Implement functionality to close window here.
}

void *SDL2Window::GetDisplay()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	// Implement functionality to get display here. Return nullptr as placeholder.
	return nullptr;
}

void *SDL2Window::GetWindow()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	// Implement functionality to get window here. Return nullptr as placeholder.
	return nullptr;
}

void SDL2Window::Tick()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			break;

		default:
			break;
		}
	}

	SDL_Delay(16); // Delay to control frame rate (approx. 60 FPS)
}

RenderDevice *SDL2Window::GetRenderDevice()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	// Implement logic for getting Render Device. Here we return nullptr as
	// placeholder.
	return rendDevice.get();
}
