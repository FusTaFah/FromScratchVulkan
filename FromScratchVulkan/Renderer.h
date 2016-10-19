#pragma once

#include <vulkan\vulkan.h>

class Renderer {
public:
	Renderer();
	~Renderer();
private:
	void _InitInstance();
	void _DeInitInstance();

	void _InitDevice();
	void _DeInitDevice();

	VkInstance _instance = nullptr;
	VkPhysicalDevice _gpu = nullptr;
	VkDevice _device = nullptr;
	uint32_t _graphics_family_index = 0;
};