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
	InitShaders();
	
}

Renderer::~Renderer() {
	DeInitPipeline();
	DeInitVertexBuffer();
	DeInitFrameBuffer();
	DeInitShaders();
	DeInitRenderPass();
	delete m_pipeline;
	DeInitCommandBuffer();
	delete m_window;
	DeInitDevice();
	DeInitDebug();
	DeInitInstance();
}

Window * Renderer::CreateVulkanWindow(uint32_t size_x, uint32_t size_y, std::string name) {
	m_window = new Window(this, size_x, size_y, name);
	InitRenderPass();
	InitFrameBuffer();
	InitVertexBuffer();
	InitPipeline();
	return m_window;
}

bool Renderer::Run() {
	if (m_window != nullptr) {
		return m_window->Update();
	}
	return true;
}

void Renderer::BeginCommandBuffer(uint32_t buffer_number) {
	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	ErrorCheck(vkBeginCommandBuffer(m_command_buffer[buffer_number], &command_buffer_begin_info));
}

void Renderer::EndCommandBuffer(uint32_t buffer_number) {
	ErrorCheck(vkEndCommandBuffer(m_command_buffer[buffer_number]));
}

void Renderer::QueueCommandBuffer(uint32_t buffer_number) {
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffer[buffer_number];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &m_semaphore;
	
	ErrorCheck(vkQueueSubmit(m_queue, 1, &submit_info, VK_NULL_HANDLE));
}

void Renderer::QueueCommandBuffer(uint32_t buffer_number, VkPipelineStageFlags flags[]) {
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffer[buffer_number];
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &m_semaphore;
	submit_info.pWaitDstStageMask = flags;
	ErrorCheck(vkQueueSubmit(m_queue, 1, &submit_info, VK_NULL_HANDLE));
}

void Renderer::WaitCommandBuffer() {
	ErrorCheck(vkQueueWaitIdle(m_queue));
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
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 30);
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
	WaitCommandBuffer();
	vkDestroyCommandPool(m_device, m_command_pool, nullptr);
	vkDestroyFence(m_device, m_fence, nullptr);
	vkDestroySemaphore(m_device, m_semaphore, nullptr);
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

	BeginCommandBuffer(0);

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
		image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	VkPipelineStageFlags source_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(m_command_buffer[0], source_stages, destination_stages, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	EndCommandBuffer(0);
	//WaitCommandBuffer();
	VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	QueueCommandBuffer(0, flags);
	

	VkAttachmentDescription attachment_descriptions[2];
	attachment_descriptions[0].format = m_window->GetSurfaceFormatKHR().format;
	attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachment_descriptions[0].flags = 0;

	attachment_descriptions[1].format = VK_FORMAT_D16_UNORM;
	attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment_descriptions[1].flags = 0;

	VkAttachmentReference color_attachment_reference{};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_reference{};
	depth_attachment_reference.attachment = 1;
	depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description{};
	subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.flags = 0;
	subpass_description.inputAttachmentCount = 0;
	subpass_description.pInputAttachments = VK_NULL_HANDLE;
	subpass_description.colorAttachmentCount = 1;
	subpass_description.pColorAttachments = &color_attachment_reference;
	subpass_description.pResolveAttachments = VK_NULL_HANDLE;
	subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments = VK_NULL_HANDLE;

	VkRenderPassCreateInfo render_pass_create_info{};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.pNext = VK_NULL_HANDLE;
	render_pass_create_info.attachmentCount = 2;
	render_pass_create_info.pAttachments = attachment_descriptions;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass_description;
	render_pass_create_info.dependencyCount = 0;
	render_pass_create_info.pDependencies = VK_NULL_HANDLE;
	ErrorCheck(vkCreateRenderPass(m_device, &render_pass_create_info, VK_NULL_HANDLE, &m_render_pass));
}

void Renderer::DeInitRenderPass() {
	vkDestroyRenderPass(m_device, m_render_pass, VK_NULL_HANDLE);
}

void Renderer::InitShaders() {
	static const char * vertex_shader_text =
		"#version 400\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"#extension GL_ARB_shading_language_420pack : enable\n"
		"layout (std140, binding = 0) uniform bufferVals {\n"
		"    mat4 mvp;\n"
		"} myBufferVals;\n"
		"layout (location = 0) in vec4 pos;\n"
		"layout (location = 1) in vec4 inColor;\n"
		"layout (location = 0) out vec4 outColor;\n"
		"out gl_PerVertex { \n"
		"    vec4 gl_Position;\n"
		"};\n"
		"void main() {\n"
		"   outColor = inColor;\n"
		"   gl_Position = myBufferVals.mvp * pos;\n"
		"}\n";

	static const char * fragment_shader_text =
		"#version 400\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"#extension GL_ARB_shading_language_420pack : enable\n"
		"layout (location = 0) in vec4 color;\n"
		"layout (location = 0) out vec4 outColor;\n"
		"void main() {\n"
		"   outColor = color;\n"
		"}\n";

	std::vector<unsigned int> vertex_shader_SPIR_V;
	m_pipeline_shader_stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_pipeline_shader_stage_create_info[0].pNext = VK_NULL_HANDLE;
	m_pipeline_shader_stage_create_info[0].pSpecializationInfo = VK_NULL_HANDLE;
	m_pipeline_shader_stage_create_info[0].flags = 0;
	m_pipeline_shader_stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	m_pipeline_shader_stage_create_info[0].pName = "main";

	glslang::InitializeProcess();

	bool vertex_shader_result = GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_text, vertex_shader_SPIR_V);
	if (!vertex_shader_result) {
		assert(0 && "Shader could not be converted from GLSL to SPIR_V");
	}

	VkShaderModuleCreateInfo vertex_shader_module_create_info{};
	vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertex_shader_module_create_info.pNext = VK_NULL_HANDLE;
	vertex_shader_module_create_info.flags = 0;
	vertex_shader_module_create_info.codeSize = vertex_shader_SPIR_V.size() * sizeof(unsigned int);
	vertex_shader_module_create_info.pCode = vertex_shader_SPIR_V.data();

	ErrorCheck(vkCreateShaderModule(m_device, &vertex_shader_module_create_info, VK_NULL_HANDLE, &m_pipeline_shader_stage_create_info[0].module));

	std::vector<unsigned int> fragment_shader_SPIR_V;
	m_pipeline_shader_stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_pipeline_shader_stage_create_info[1].pNext = VK_NULL_HANDLE;
	m_pipeline_shader_stage_create_info[1].pSpecializationInfo = VK_NULL_HANDLE;
	m_pipeline_shader_stage_create_info[1].flags = 0;
	m_pipeline_shader_stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_pipeline_shader_stage_create_info[1].pName = "main";

	bool fragment_shader_result = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_text, fragment_shader_SPIR_V);
	if (!fragment_shader_result) {
		assert(0 && "Shader could not be converted from GLSL to SPIR_V");
	}

	VkShaderModuleCreateInfo fragment_shader_module_create_info{};
	fragment_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragment_shader_module_create_info.pNext = VK_NULL_HANDLE;
	fragment_shader_module_create_info.flags = 0;
	fragment_shader_module_create_info.codeSize = fragment_shader_SPIR_V.size() * sizeof(unsigned int);
	fragment_shader_module_create_info.pCode = fragment_shader_SPIR_V.data();

	ErrorCheck(vkCreateShaderModule(m_device, &fragment_shader_module_create_info, VK_NULL_HANDLE, &m_pipeline_shader_stage_create_info[1].module));

	glslang::FinalizeProcess();
}

void Renderer::DeInitShaders() {
	for (int i = 0; i < 2; i++) {
		vkDestroyShaderModule(m_device, m_pipeline_shader_stage_create_info[i].module, VK_NULL_HANDLE);
	}
}

void Renderer::InitFrameBuffer() {
	WaitCommandBuffer();

	BeginCommandBuffer(0);
	VkImageView frame_buffer_views[2];
	frame_buffer_views[1] = m_window->GetDepthBuffer();

	VkFramebufferCreateInfo frame_buffer_create_info{};
	frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frame_buffer_create_info.pNext = VK_NULL_HANDLE;
	frame_buffer_create_info.renderPass = m_render_pass;
	frame_buffer_create_info.attachmentCount = 2;
	frame_buffer_create_info.pAttachments = frame_buffer_views;
	frame_buffer_create_info.width = m_window->GetSurfaceSizeX();
	frame_buffer_create_info.height = m_window->GetSurfaceSizeY();
	frame_buffer_create_info.layers = 1;

	m_frame_buffers = (VkFramebuffer *)malloc(m_window->GetSwapchainImageCount() * sizeof(VkFramebuffer));
	if (m_frame_buffers == 0) {
		assert(0 && "Could not allocate memory for frame buffers; either they haven't been created or your OS is fucked");
	}

	for (int i = 0; i < m_window->GetSwapchainImageCount(); i++) {
		frame_buffer_views[0] = m_window->GetSwapchainImageViews()[i];
		ErrorCheck(vkCreateFramebuffer(m_device, &frame_buffer_create_info, VK_NULL_HANDLE, &m_frame_buffers[i]));
	}

	EndCommandBuffer(0);
	//we aren't executing Cmd(s) so chill with the pipeline stage b0ss
	//VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	QueueCommandBuffer(0);
}

void Renderer::DeInitFrameBuffer() {
	for (int i = 0; i < m_window->GetSwapchainImageCount(); i++) {
		vkDestroyFramebuffer(m_device, m_frame_buffers[i], VK_NULL_HANDLE);
	}
	free(m_frame_buffers);
}

void Renderer::InitVertexBuffer() {
	BeginCommandBuffer(0);

	const vertex_data g_vbData[] = {
		vertex_data(glm::vec3(-1, -1, -1), glm::vec3(0.f, 0.f, 0.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 1.f, 0.f)),

		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(1,  1,  1), glm::vec3(1.f, 1.f, 1.f)),

		vertex_data(glm::vec3(1,  1,  1), glm::vec3(1.f, 1.f, 1.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(1.f, 0.f, 0.f)),

		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1, -1, -1), glm::vec3(0.f, 0.f, 0.f)),

		vertex_data(glm::vec3(1,  1,  1), glm::vec3(1.f, 1.f, 1.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),

		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(-1, -1, -1), glm::vec3(0.f, 0.f, 0.f)),
	};

	const vertex_data g_vb_solid_face_colors_Data[] = {
		//red face
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(1.f, 0.f, 0.f)),
		vertex_data(glm::vec3(1,  1,  1), glm::vec3(1.f, 0.f, 0.f)),
		//green face
		vertex_data(glm::vec3(-1, -1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(0.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(0.f, 1.f, 0.f)),
		//blue face
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1, -1, -1), glm::vec3(0.f, 0.f, 1.f)),
		//yellow face
		vertex_data(glm::vec3(1,  1,  1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 1.f, 0.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(1.f, 1.f, 0.f)),
		//magenta face
		vertex_data(glm::vec3(1,  1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(1,  1, -1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1,  1), glm::vec3(1.f, 0.f, 1.f)),
		vertex_data(glm::vec3(-1,  1, -1), glm::vec3(1.f, 0.f, 1.f)),
		//cyan face
		vertex_data(glm::vec3(1, -1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(-1, -1,  1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(1, -1, -1), glm::vec3(0.f, 1.f, 1.f)),
		vertex_data(glm::vec3(-1, -1, -1), glm::vec3(0.f, 1.f, 1.f))
	};

	const vertex_uv_data gg[] = {
		//left face
		vertex_uv_data(glm::vec3(-1, -1, -1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(-1,  1,  1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(-1, -1,  1), glm::vec2(0.f, 0.f)),
		vertex_uv_data(glm::vec3(-1,  1,  1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(-1, -1, -1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(-1,  1, -1), glm::vec2(1.f, 1.f)),
		// front face
		vertex_uv_data(glm::vec3(-1, -1, -1), glm::vec2(0.f, 0.f)),
		vertex_uv_data(glm::vec3(1, -1, -1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1, -1), glm::vec2(1.f, 1.f)),
		vertex_uv_data(glm::vec3(-1, -1, -1), glm::vec2(0.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1, -1), glm::vec2(1.f, 1.f)),
		vertex_uv_data(glm::vec3(-1,  1, -1), glm::vec2(0.f, 1.f)),
		// top face
		vertex_uv_data(glm::vec3(-1, -1, -1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(1, -1,  1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(1, -1, -1), glm::vec2(1.f, 1.f)),
		vertex_uv_data(glm::vec3(-1, -1, -1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(-1, -1,  1), glm::vec2(0.f, 0.f)),
		vertex_uv_data(glm::vec3(1, -1,  1), glm::vec2(1.f, 0.f)),
		// bottom face
		vertex_uv_data(glm::vec3(-1,  1, -1), glm::vec2(0.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1,  1), glm::vec2(1.f, 1.f)),
		vertex_uv_data(glm::vec3(-1,  1,  1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(-1,  1, -1), glm::vec2(0.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1, -1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1,  1), glm::vec2(1.f, 1.f)),
		// right face
		vertex_uv_data(glm::vec3(1,  1, -1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(1, -1,  1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1,  1), glm::vec2(1.f, 1.f)),
		vertex_uv_data(glm::vec3(1, -1,  1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1, -1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(1, -1, -1), glm::vec2(0.f, 0.f)),
		// back face
		vertex_uv_data(glm::vec3(-1,  1,  1), glm::vec2(1.f, 1.f)),
		vertex_uv_data(glm::vec3(1,  1,  1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(-1, -1,  1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(-1, -1,  1), glm::vec2(1.f, 0.f)),
		vertex_uv_data(glm::vec3(1,  1,  1), glm::vec2(0.f, 1.f)),
		vertex_uv_data(glm::vec3(1, -1,  1), glm::vec2(0.f, 0.f)),
	};

	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.pNext = VK_NULL_HANDLE;
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_create_info.size = sizeof(g_vb_solid_face_colors_Data);
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = VK_NULL_HANDLE;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.flags = 0;

	ErrorCheck(vkCreateBuffer(m_device, &buffer_create_info, VK_NULL_HANDLE, &m_vertex_buffer));

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(m_device, m_vertex_buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = VK_NULL_HANDLE;
	memory_allocate_info.memoryTypeIndex = 0;
	memory_allocate_info.allocationSize = memory_requirements.size;

	memory_types_from_properties(memory_requirements.memoryTypeBits, 0, &memory_allocate_info.memoryTypeIndex, m_gpu_memory_properties);

	ErrorCheck(vkAllocateMemory(m_device, &memory_allocate_info, VK_NULL_HANDLE, &m_vertex_buffer_memory));

	uint8_t *pData;

	ErrorCheck(vkMapMemory(m_device, m_vertex_buffer_memory, 0, memory_requirements.size, 0, (void**)&pData));

	memcpy(pData, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data));

	vkUnmapMemory(m_device, m_vertex_buffer_memory);

	ErrorCheck(vkBindBufferMemory(m_device, m_vertex_buffer, m_vertex_buffer_memory, 0));

	m_vertex_input_binding_description.binding = 0;
	m_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_vertex_input_binding_description.stride = sizeof(g_vb_solid_face_colors_Data[0]);

	m_vertex_input_attribute_descriptions[0].binding = 0;
	m_vertex_input_attribute_descriptions[0].location = 0;
	m_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertex_input_attribute_descriptions[0].offset = 0;
	m_vertex_input_attribute_descriptions[1].binding = 0;
	m_vertex_input_attribute_descriptions[1].location = 1;
	m_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertex_input_attribute_descriptions[1].offset = sizeof(glm::vec3);

	const VkDeviceSize device_size_offsets[1] = { 0 };

	VkClearValue clear_values[2];
	clear_values[0].color.float32[0] = 0.2f;
	clear_values[0].color.float32[1] = 0.2f;
	clear_values[0].color.float32[2] = 0.2f;
	clear_values[0].color.float32[3] = 0.2f;
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	VkSemaphore image_aquired_semaphore;
	VkSemaphoreCreateInfo image_aquired_semaphore_create_info{};
	image_aquired_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	image_aquired_semaphore_create_info.pNext = VK_NULL_HANDLE;
	image_aquired_semaphore_create_info.flags = 0;

	ErrorCheck(vkCreateSemaphore(m_device, &image_aquired_semaphore_create_info, VK_NULL_HANDLE, &image_aquired_semaphore));

	uint32_t current_image;

	ErrorCheck(vkAcquireNextImageKHR(m_device, m_window->GetSwapchain(), UINT64_MAX, image_aquired_semaphore, VK_NULL_HANDLE, &current_image));

	VkImageMemoryBarrier image_memory_barrier{};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = VK_NULL_HANDLE;
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = m_window->GetSwapchainImages()[current_image];
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags source_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(m_command_buffer[0], source_stages, destination_stages, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &image_memory_barrier);

	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = VK_NULL_HANDLE;
	render_pass_begin_info.renderPass = m_render_pass;
	render_pass_begin_info.framebuffer = m_frame_buffers[current_image];
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = m_window->GetSurfaceSizeX();
	render_pass_begin_info.renderArea.extent.height = m_window->GetSurfaceSizeY();
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(m_command_buffer[0], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindVertexBuffers(m_command_buffer[0], 0, 1, &m_vertex_buffer, device_size_offsets);

	vkCmdEndRenderPass(m_command_buffer[0]);

	EndCommandBuffer(0);

	QueueCommandBuffer(0);

	vkDestroySemaphore(m_device, image_aquired_semaphore, VK_NULL_HANDLE);
}

void Renderer::DeInitVertexBuffer() {
	vkDestroyBuffer(m_device, m_vertex_buffer, VK_NULL_HANDLE);
	//free(&m_vertex_buffer);
	vkFreeMemory(m_device, m_vertex_buffer_memory, VK_NULL_HANDLE);
}

void Renderer::InitPipeline() {
	VkDynamicState dynamic_states[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo pipeline_dynamic_stage_create_info{};
	memset(dynamic_states, 0, sizeof dynamic_states);
	pipeline_dynamic_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipeline_dynamic_stage_create_info.pNext = VK_NULL_HANDLE;
	pipeline_dynamic_stage_create_info.pDynamicStates = dynamic_states;
	pipeline_dynamic_stage_create_info.dynamicStateCount = 0;

	VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info{};
	pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipeline_vertex_input_state_create_info.pNext = VK_NULL_HANDLE;
	pipeline_vertex_input_state_create_info.flags = 0;
	pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
	pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &m_vertex_input_binding_description;
	pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
	pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = m_vertex_input_attribute_descriptions;

	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{};
	pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline_input_assembly_state_create_info.pNext = VK_NULL_HANDLE;
	pipeline_input_assembly_state_create_info.flags = 0;
	pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
	pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{};
	pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipeline_rasterization_state_create_info.pNext = VK_NULL_HANDLE;
	pipeline_rasterization_state_create_info.flags = 0;
	pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipeline_rasterization_state_create_info.depthClampEnable = VK_TRUE;
	pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0;
	pipeline_rasterization_state_create_info.depthBiasClamp = 0;
	pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0;
	pipeline_rasterization_state_create_info.lineWidth = 1.0f;

	VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info{};
	pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipeline_color_blend_state_create_info.pNext = VK_NULL_HANDLE;
	pipeline_color_blend_state_create_info.flags = 0;
	
	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state[1];
	pipeline_color_blend_attachment_state[0].colorWriteMask = 0xf;
	pipeline_color_blend_attachment_state[0].blendEnable = VK_FALSE;
	pipeline_color_blend_attachment_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
	pipeline_color_blend_attachment_state[0].colorBlendOp = VK_BLEND_OP_ADD;
	pipeline_color_blend_attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_color_blend_attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_color_blend_attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_color_blend_attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	
	pipeline_color_blend_state_create_info.attachmentCount = 1;
	pipeline_color_blend_state_create_info.pAttachments = pipeline_color_blend_attachment_state;
	pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
	pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_NO_OP;
	pipeline_color_blend_state_create_info.blendConstants[0] = 1.0f;
	pipeline_color_blend_state_create_info.blendConstants[1] = 1.0f;
	pipeline_color_blend_state_create_info.blendConstants[2] = 1.0f;
	pipeline_color_blend_state_create_info.blendConstants[3] = 1.0f;

	VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info{};
	pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_create_info.pNext = VK_NULL_HANDLE;
	pipeline_viewport_state_create_info.flags = 0;
	pipeline_viewport_state_create_info.viewportCount = 1;
	dynamic_states[pipeline_dynamic_stage_create_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	pipeline_viewport_state_create_info.scissorCount = 1;
	dynamic_states[pipeline_dynamic_stage_create_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
	pipeline_viewport_state_create_info.pScissors = VK_NULL_HANDLE;
	pipeline_viewport_state_create_info.pViewports = VK_NULL_HANDLE;

	VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info{};
	pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipeline_depth_stencil_state_create_info.pNext = VK_NULL_HANDLE;
	pipeline_depth_stencil_state_create_info.flags = 0;
	pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
	pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
	pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
	pipeline_depth_stencil_state_create_info.minDepthBounds = 0;
	pipeline_depth_stencil_state_create_info.maxDepthBounds = 0;
	pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
	pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
	pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
	pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
	pipeline_depth_stencil_state_create_info.back.compareMask = 0;
	pipeline_depth_stencil_state_create_info.back.reference = 0;
	pipeline_depth_stencil_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
	pipeline_depth_stencil_state_create_info.back.writeMask = 0;
	pipeline_depth_stencil_state_create_info.front = pipeline_depth_stencil_state_create_info.back;

	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info{};
	pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipeline_multisample_state_create_info.pNext = VK_NULL_HANDLE;
	pipeline_multisample_state_create_info.flags = 0;
	pipeline_multisample_state_create_info.pSampleMask = VK_NULL_HANDLE;
	pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
	pipeline_multisample_state_create_info.minSampleShading = 0.0f;

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.pNext = VK_NULL_HANDLE;
	graphics_pipeline_create_info.layout = m_pipeline->GetPipelineLayout();
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = 0;
	graphics_pipeline_create_info.flags = 0;
	graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
	graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
	graphics_pipeline_create_info.pTessellationState = VK_NULL_HANDLE;
	graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
	graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_stage_create_info;
	graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
	graphics_pipeline_create_info.pStages = m_pipeline_shader_stage_create_info;
	graphics_pipeline_create_info.stageCount = 2;
	graphics_pipeline_create_info.renderPass = m_render_pass;
	graphics_pipeline_create_info.subpass = 0;

	ErrorCheck(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, VK_NULL_HANDLE, &m_graphics_pipeline));
}

void Renderer::DeInitPipeline() {
	vkDestroyPipeline(m_device, m_graphics_pipeline, VK_NULL_HANDLE);
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

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
	m_instance_layer_list.push_back("VK_LAYER_NV_optimus");
	m_instance_extention_list.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_api_dump");
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_core_validation");
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_image");
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_object_tracker");
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_parameter_validation");
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_screenshot");
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_swapchain");
	//m_instance_layer_list.push_back("VK_LAYER_GOOGLE_threading");
	//m_instance_layer_list.push_back("VK_LAYER_GOOGLE_unique_objects");
	////m_instance_layer_list.push_back("VK_LAYER_LUNARG_vktrace");
	//m_instance_layer_list.push_back("VK_LAYER_RENDERDOC_Capture");
	//m_instance_layer_list.push_back("VK_LAYER_VALVE_steam_overlay");
	//m_instance_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
	//m_device_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
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