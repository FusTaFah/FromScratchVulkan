#pragma once

#include "Platform.h"
#include <string>
#include <vector>
#include "Pipeline.h"

class Renderer;

class Window {
public:
	Window(Renderer * renderer, uint32_t size_x, uint32_t size_y, std::string name);
	~Window();
	void Close();
	bool Update();
	VkSwapchainKHR & GetSwapchain();

private:
	void InitOSWindow();
	void DeInitOSWindow();
	void UpdateOSWindow();
	void InitOSSurface();

	void InitSurface();
	void DeInitSurface();

	void InitSwapchain();
	void DeInitSwapchain();

	void InitSwapchainImages();
	void DeInitSwapchainImages();

	void InitDepthBuffer();
	void DeInitDepthBuffer();

	Renderer * m_renderer;

	VkSurfaceKHR m_surface;

	VkSwapchainKHR m_swapchain;

	uint32_t m_surface_size_x = 512;
	uint32_t m_surface_size_y = 512;
	std::string m_window_name;
	uint32_t m_swapchain_image_count;

	VkSurfaceCapabilitiesKHR m_surface_capabilities;
	VkSurfaceFormatKHR m_surface_format;

	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;

	VkImage m_image;
	VkImageView m_image_view;

	VkDeviceMemory m_depth_buffer_memory;

	//VkBuffer m_buffer;

	//VkDeviceMemory m_uniform_buffer_memory;

	//VkDescriptorBufferInfo m_buffer_info;

	//std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
	//VkPipelineLayout m_pipeline_layout;
	//VkDescriptorPool m_descriptor_pool;

	bool m_running = true;

	Pipeline * m_pipeline;

#ifdef VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE m_win32_instance = NULL;
	HWND m_win32_window = NULL;
	std::string m_win32_class_name;
	static uint64_t m_win32_class_id_counter;
#endif
};