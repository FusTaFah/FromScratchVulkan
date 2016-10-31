#pragma once

#include "Platform.h"
#include <string>

class Renderer;

class Window {
public:
	Window(Renderer * renderer, uint32_t size_x, uint32_t size_y, std::string name);
	~Window();
	void Close();
	bool Update();
private:
	void InitOSWindow();
	void DeInitOSWindow();
	void UpdateOSWindow();
	void InitOSSurface();

	void InitSurface();
	void DeInitSurface();

	void InitSwapchain();
	void DeInitSwapchain();

	Renderer * m_renderer;

	VkSurfaceKHR m_surface;

	VkSwapchainKHR m_swapchain;

	uint32_t m_surface_size_x = 512;
	uint32_t m_surface_size_y = 512;
	std::string m_window_name;
	uint32_t m_swapchain_image_count;

	VkSurfaceCapabilitiesKHR m_surface_capabilities;
	VkSurfaceFormatKHR m_surface_format;

	bool m_running = true;

#ifdef VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE m_win32_instance = NULL;
	HWND m_win32_window = NULL;
	std::string m_win32_class_name;
	static uint64_t m_win32_class_id_counter;
#endif
};