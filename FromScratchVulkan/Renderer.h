#pragma once

#include "Platform.h"
#include <vector>

class Window;

class Renderer {
public:
	Renderer();
	~Renderer();

	Window * CreateVulkanWindow(uint32_t size_x, uint32_t size_y, std::string name);
	bool Run();
//private:
	void InitInstance();
	void DeInitInstance();

	void InitDevice();
	void DeInitDevice();

	void SetupDebug();
	void InitDebug();
	void DeInitDebug();

	VkInstance m_instance;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkQueue m_queue;
	uint32_t m_graphics_family_index;
	std::vector<const char *> m_instance_layer_list;
	std::vector<const char *> m_instance_extention_list;
	std::vector<const char *> m_device_layer_list;
	std::vector<const char *> m_device_extention_list;
	VkDebugReportCallbackEXT m_debug_report;
	VkDebugReportCallbackCreateInfoEXT m_debug_report_callback_create_info;
	Window * m_window;
};