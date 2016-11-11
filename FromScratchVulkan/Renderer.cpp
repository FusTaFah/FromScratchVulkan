#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Renderer.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "Shared.h"
#include "Window.h"
#include "Pipeline.h"

Renderer::Renderer() {
	m_instance = VK_NULL_HANDLE;
	m_gpu = VK_NULL_HANDLE;
	m_device = VK_NULL_HANDLE;
	m_queue = VK_NULL_HANDLE;
	m_gpu_properties = {};
	m_gpu_memory_properties = {};
	m_graphics_family_index = 0;
	m_instance_layer_list = {};
	m_instance_extention_list = {};
	m_device_layer_list = {};
	m_device_extention_list = {};
	m_debug_report = VK_NULL_HANDLE;
	m_debug_report_callback_create_info = {};
	m_window = nullptr;

	SetupLayersAndExtentions();
	SetupDebug();
	InitInstance();
	InitDebug();
	InitDevice();
	InitCommandBuffer();
	m_pipeline = new Pipeline(this);
}

Renderer::~Renderer() {
	delete m_pipeline;
	DeInitCommandBuffer();
	delete m_window;
	DeInitDevice();
	DeInitDebug();
	DeInitInstance();
}

Window * Renderer::CreateVulkanWindow(uint32_t size_x, uint32_t size_y, std::string name) {
	m_window = new Window(this, size_x, size_y, name);
	return m_window;
}

bool Renderer::Run() {
	if (m_window != nullptr) {
		return m_window->Update();
	}
	return true;
}

void Renderer::InitRenderPass() {
	ErrorCheck(vkAcquireNextImageKHR(m_device, m_window->GetSwapchain(), UINT64_MAX, m_semaphore, VK_NULL_HANDLE, &m_current_buffer));
	//perform a bunch of random ass checks
	if (m_command_buffer[0] == VK_NULL_HANDLE) {
		assert(0 && "Command buffer not initialised");
	}
	if (m_queue == VK_NULL_HANDLE) {
		assert(0 && "Device queue not found");
	}

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageLayout old_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout new_image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkImageMemoryBarrier image_memory_barrier{};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = VK_NULL_HANDLE;
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = 0;
	image_memory_barrier.oldLayout = old_image_layout;
	image_memory_barrier.newLayout = new_image_layout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = m_window->GetSwapchainImages()[m_current_buffer];
	image_memory_barrier.subresourceRange.aspectMask = aspectMask;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	if (old_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {

	}
}

void Renderer::DeInitRenderPass() {

}

const VkInstance Renderer::GetVulkanInstance() const {
	return m_instance;
}

const VkPhysicalDevice Renderer::GetVulkanPhysicalDevice() const {
	return m_gpu;
}

const VkDevice Renderer::GetVulkanDevice() const {
	return m_device;
}

const VkQueue Renderer::GetVulkanQueue() const {
	return m_queue;
}

const VkPhysicalDeviceProperties & Renderer::GetVulkanPhysicalDeviceProperties() const {
	return m_gpu_properties;
}

VkPhysicalDeviceMemoryProperties & Renderer::GetPhysicalDeviceMemoryProperties()  {
	return m_gpu_memory_properties;
}

const uint32_t Renderer::GetVulkanGraphicsQueueFamilyIndex() const {
	return m_graphics_family_index;
}

void Renderer::SetupLayersAndExtentions() {
//	m_instance_extention_list.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
	m_instance_extention_list.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	m_instance_extention_list.push_back(PLATFORM_SURFACE_EXTENTION_NAME);
	m_device_extention_list.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Renderer::InitInstance()
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
	vkInfo.enabledLayerCount = m_instance_layer_list.size();
	vkInfo.ppEnabledLayerNames = m_instance_layer_list.data();
	vkInfo.enabledExtensionCount = m_instance_extention_list.size();
	vkInfo.ppEnabledExtensionNames = m_instance_extention_list.data();
	vkInfo.pNext = &m_debug_report_callback_create_info;

	ErrorCheck(vkCreateInstance(&vkInfo, nullptr, &m_instance));
}

void Renderer::DeInitInstance() {
	vkDestroyInstance(m_instance, nullptr);
	m_instance = nullptr;
}

void Renderer::InitDevice() {
	{
		uint32_t gpu_count = 0;
		vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);
		std::vector<VkPhysicalDevice> gpu_list(gpu_count);
		vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpu_list.data());
		m_gpu = gpu_list[0];
		vkGetPhysicalDeviceProperties(m_gpu, &m_gpu_properties);
		vkGetPhysicalDeviceMemoryProperties(m_gpu, &m_gpu_memory_properties);
	}
	{
		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &family_count, nullptr);
		std::vector<VkQueueFamilyProperties> family_property_list(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &family_count, family_property_list.data());

		bool found = false;

		for (uint32_t i = 0; i < family_property_list.size(); i++) {
			if (family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				found = true;
				m_graphics_family_index = i;
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
		vkEnumerateDeviceLayerProperties(m_gpu, &layer_count, nullptr);
		std::vector<VkLayerProperties> layer_properties(layer_count);
		vkEnumerateDeviceLayerProperties(m_gpu, &layer_count, layer_properties.data());
		std::cout << "Device Layers:" << std::endl;
		for (auto p = layer_properties.begin(); p != layer_properties.end(); ++p) {
			std::cout << p->layerName << "\t\t | " << p->description << std::endl;
		}
		std::cout << std::endl;
	}

	float queue_priorities[]{ 1.0f };
	VkDeviceQueueCreateInfo device_queue_info{};
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.queueFamilyIndex = m_graphics_family_index;
	device_queue_info.queueCount = 1;
	device_queue_info.pQueuePriorities = queue_priorities;

	VkDeviceCreateInfo device_info{};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &device_queue_info;
	device_info.enabledLayerCount = m_device_layer_list.size();
	device_info.ppEnabledLayerNames = m_device_layer_list.data();
	device_info.enabledExtensionCount = m_device_extention_list.size();
	device_info.ppEnabledExtensionNames = m_device_extention_list.data();

	ErrorCheck(vkCreateDevice(m_gpu, &device_info, nullptr, &m_device));

	vkGetDeviceQueue(m_device, m_graphics_family_index, 0, &m_queue);
}

void Renderer::DeInitDevice() {
	vkDestroyDevice(m_device, nullptr);
	m_device = nullptr;
}

void Renderer::InitCommandBuffer() {
	VkFenceCreateInfo fence_create_info{};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	ErrorCheck(vkCreateFence(m_device, &fence_create_info, nullptr, &m_fence));
	
	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	ErrorCheck(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_semaphore));

	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = m_graphics_family_index;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	ErrorCheck(vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool));

	
	VkCommandBufferAllocateInfo command_buffer_info{};
	command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_info.commandPool = m_command_pool;
	command_buffer_info.commandBufferCount = 2;
	command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ErrorCheck(vkAllocateCommandBuffers(m_device, &command_buffer_info, m_command_buffer));

	//{
	//	VkCommandBufferBeginInfo command_buffer_begin_info{};
	//	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//	vkBeginCommandBuffer(command_buffer[0], &command_buffer_begin_info);

	//	vkCmdPipelineBarrier(command_buffer[0],
	//		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	//		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	//		0,
	//		0, nullptr,
	//		0, nullptr,
	//		0, nullptr);

	//	//command buffer... commands

	//	VkViewport viewport{};
	//	viewport.width = 1366.0f;
	//	viewport.height = 768.0f;
	//	viewport.x = 0.0f;
	//	viewport.y = 0.0f;
	//	viewport.maxDepth = 1.0f;
	//	viewport.minDepth = 0.0f;
	//	vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);

	//	vkEndCommandBuffer(command_buffer[0]);
	//}

	//{
	//	VkCommandBufferBeginInfo command_buffer_begin_info{};
	//	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//	vkBeginCommandBuffer(command_buffer[1], &command_buffer_begin_info);

	//	//command buffer... commands

	//	VkViewport viewport{};
	//	viewport.width = 1366.0f;
	//	viewport.height = 768.0f;
	//	viewport.x = 0.0f;
	//	viewport.y = 0.0f;
	//	viewport.maxDepth = 1.0f;
	//	viewport.minDepth = 0.0f;
	//	vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);

	//	vkEndCommandBuffer(command_buffer[1]);
	//}

	//{
	//	VkSubmitInfo submit_info{};
	//	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	submit_info.commandBufferCount = 1;
	//	submit_info.pCommandBuffers = &command_buffer[0];
	//	submit_info.signalSemaphoreCount = 1;
	//	submit_info.pSignalSemaphores = &semaphore;
	//	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	//}

	//{
	//	VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	//	VkSubmitInfo submit_info{};
	//	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	submit_info.commandBufferCount = 1;
	//	submit_info.pCommandBuffers = &command_buffer[1];
	//	submit_info.waitSemaphoreCount = 1;
	//	submit_info.pWaitSemaphores = &semaphore;
	//	submit_info.pWaitDstStageMask = flags;
	//	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	//}

	////vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	//vkQueueWaitIdle(queue);

	
}

void Renderer::DeInitCommandBuffer() {
	vkDestroyCommandPool(m_device, m_command_pool, nullptr);
	vkDestroyFence(m_device, m_fence, nullptr);
	vkDestroySemaphore(m_device, m_semaphore, nullptr);
}

#if BUILD_OPTIONS_DEBUG

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

void Renderer::SetupDebug() {
	m_debug_report_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	m_debug_report_callback_create_info.pfnCallback = VulkanDebugCallback;
	m_debug_report_callback_create_info.flags =
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT |
		0;

	m_instance_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
	m_instance_extention_list.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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
	m_device_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::InitDebug() {
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
	if (fvkCreateDebugReportCallbackEXT == nullptr || fvkDestroyDebugReportCallbackEXT == nullptr) {
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointers");
		std::exit(-1);
	}

	fvkCreateDebugReportCallbackEXT(m_instance, &m_debug_report_callback_create_info, nullptr, &m_debug_report);
}

void Renderer::DeInitDebug() {
	fvkDestroyDebugReportCallbackEXT(m_instance, m_debug_report, nullptr);
	m_debug_report = VK_NULL_HANDLE;
}

#else

void Renderer::_InitDebug() {}
void Renderer::_SetupDebug() {}
void Renderer::_DeInitDebug() {}

#endif // BUILD_OPTIONS_DEBUG