#include "Renderer.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "Shared.h"

#ifdef _WIN32
#include <windows.h>
#endif


Renderer::Renderer() {
	_SetupDebug();
	_InitInstance();
	_InitDebug();
	_InitDevice();
}

Renderer::~Renderer() {
	_DeInitDevice();
	_DeInitDebug();
	_DeInitInstance();
	
}

void Renderer::_InitInstance()
{
	//remember to always default initialise with the struct operator{}
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 3);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "Vulkan First";

	VkInstanceCreateInfo vkInfo{};
	vkInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkInfo.pApplicationInfo = &appInfo;
	vkInfo.enabledLayerCount = _instance_layer_list.size();
	vkInfo.ppEnabledLayerNames = _instance_layer_list.data();
	vkInfo.enabledExtensionCount = _instance_extention_list.size();
	vkInfo.ppEnabledExtensionNames = _instance_extention_list.data();
	vkInfo.pNext = &debug_report_callback_create_info;

	ErrorCheck(vkCreateInstance(&vkInfo, nullptr, &_instance));
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
			if (family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
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

	{
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> layer_properties(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, layer_properties.data());
		std::cout << "Instance Layers:" << std::endl;
		for (auto p = layer_properties.begin(); p != layer_properties.end(); ++p) {
			std::cout << p->layerName << "\t\t | " << p->description << std::endl;
		}
		std::cout << std::endl;
	}

	{
		uint32_t layer_count = 0;
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, nullptr);
		std::vector<VkLayerProperties> layer_properties(layer_count);
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, layer_properties.data());
		std::cout << "Device Layers:" << std::endl;
		for (auto p = layer_properties.begin(); p != layer_properties.end(); ++p) {
			std::cout << p->layerName << "\t\t | " << p->description << std::endl;
		}
		std::cout << std::endl;
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
	device_info.enabledLayerCount = _device_layer_list.size();
	device_info.ppEnabledLayerNames = _device_layer_list.data();
	device_info.enabledExtensionCount = _device_extention_list.size();
	device_info.ppEnabledExtensionNames = _device_extention_list.data();

	ErrorCheck(vkCreateDevice(_gpu, &device_info, nullptr, &_device));

	vkGetDeviceQueue(_device, _graphics_family_index, 0, &_queue);
}

void Renderer::_DeInitDevice() {
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
	VkDebugReportFlagsEXT msg_flags,
	VkDebugReportObjectTypeEXT obj_type,
	uint64_t source_object,
	size_t location,
	int32_t msg_code,
	const char * layer_prefix,
	const char * msg,
	void * user_data
	)
{
	std::ostringstream stream;
	stream << "VKDBG: ";
	if (msg_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		stream << "INFORMATION: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		stream << "WARNING: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		stream << "PERFORMANCE WARNING: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		stream << "ERROR: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		stream << "DEBUG: ";
	}
	stream << "@[ " << layer_prefix << " ]: ";
	stream << msg << std::endl;
	std::cout << stream.str();

#ifdef _WIN32
	if (msg_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		MessageBox(NULL, stream.str().c_str(), "Vulkan Error!", 0);
	}
#endif

	return false;
}

void Renderer::_SetupDebug() {
	debug_report_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debug_report_callback_create_info.pfnCallback = VulkanDebugCallback;
	debug_report_callback_create_info.flags =
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT |
		0;

	_instance_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
	_instance_extention_list.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	/*VK_LAYER_LUNARG_api_dump
	VK_LAYER_LUNARG_core_validation
	VK_LAYER_LUNARG_image
	VK_LAYER_LUNARG_object_tracker
	VK_LAYER_LUNARG_parameter_validation
	VK_LAYER_LUNARG_screenshot
	VK_LAYER_LUNARG_swapchain
	VK_LAYER_GOOGLE_threading
	VK_LAYER_GOOGLE_unique_objects
	VK_LAYER_LUNARG_vktrace
	VK_LAYER_RENDERDOC_Capture
	VK_LAYER_VALVE_steam_overlay
	VK_LAYER_LUNARG_standard_validation*/
	_device_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::_InitDebug() {
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");
	if (fvkCreateDebugReportCallbackEXT == nullptr || fvkDestroyDebugReportCallbackEXT == nullptr) {
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointers");
		std::exit(-1);
	}

	fvkCreateDebugReportCallbackEXT(_instance, &debug_report_callback_create_info, nullptr, &_debug_report);
}

void Renderer::_DeInitDebug() {
	fvkDestroyDebugReportCallbackEXT(_instance, _debug_report, nullptr);
	_debug_report = VK_NULL_HANDLE;
}