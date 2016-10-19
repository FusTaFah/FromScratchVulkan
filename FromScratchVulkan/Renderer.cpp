#include "Renderer.h"
#include <cstdlib>
#include <assert.h>
#include <vector>

Renderer::Renderer() {
	_InitInstance();
	_InitDevice();
}

Renderer::~Renderer() {
	_DeInitDevice();
	_DeInitInstance();
	
}

void Renderer::_InitInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 3);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "Vulkan First";

	VkInstanceCreateInfo vkInfo{};
	vkInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkInfo.pApplicationInfo = &appInfo;

	VkResult res = vkCreateInstance(&vkInfo, nullptr, &_instance);
	if (res != VK_SUCCESS) {
		assert(0 && "Vulkan instance creation error");
		std::exit(res);
	}
}

void Renderer::_DeInitInstance() {
	vkDestroyInstance(_instance, nullptr);
	_instance = nullptr;
}

void Renderer::_InitDevice() {
	{
		uint32_t gpu_count = 0;
		vkEnumeratePhysicalDevices(_instance, &gpu_count, nullptr);
		std::vector<VkPhysicalDevice> gpu_list(gpu_count);
		vkEnumeratePhysicalDevices(_instance, &gpu_count, gpu_list.data());
		_gpu = gpu_list[0];
	}
	{
		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, nullptr);
		std::vector<VkQueueFamilyProperties> family_property_list(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, family_property_list.data());

		bool found = false;

		for (uint32_t i = 0; i < family_property_list.size(); i++) {
			if (family_property_list[i].queueFlags == VK_QUEUE_GRAPHICS_BIT) {
				found = true;
				_graphics_family_index = i;
				break;
			}
		}
		if (!found) {
			assert(0 && "Vulkan ERROR: Queue Family supporting graphics not found");
			std::exit(-1);
		}
	}

	float queue_priorities[]{ 1.0f };
	VkDeviceQueueCreateInfo device_queue_info{};
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.queueFamilyIndex = _graphics_family_index;
	device_queue_info.queueCount = 1;
	device_queue_info.pQueuePriorities = queue_priorities;

	VkDeviceCreateInfo device_info{};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &device_queue_info;

	VkResult res = vkCreateDevice(_gpu, &device_info, nullptr, &_device);
	if (res != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: Could not create device");
		std::exit(res);
	}
}

void Renderer::_DeInitDevice() {
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}