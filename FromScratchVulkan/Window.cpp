#include "Window.h"
#include "Renderer.h"
#include <assert.h>
#include "Shared.h"

Window::Window(Renderer * renderer, uint32_t size_x, uint32_t size_y, std::string name) :
	m_surface_size_x(size_x),
	m_surface_size_y(size_y),
	m_window_name(name),
	m_renderer(renderer),
	m_surface(VK_NULL_HANDLE),
	m_surface_capabilities({}),
	m_surface_format({}),
	m_swapchain(VK_NULL_HANDLE),
	m_swapchain_image_count(2)
{
	InitOSWindow();
	InitSurface();
	InitSwapchain();
	InitSwapchainImages();
	InitDepthBuffer();
}

Window::~Window() {
	DeInitDepthBuffer();
	DeInitSwapchainImages();
	DeInitSwapchain();
	DeInitSurface();
	DeInitOSWindow();
}

void Window::Close() {
	m_running = false;
}

bool Window::Update() {
	UpdateOSWindow();
	return m_running;
}

VkSwapchainKHR & Window::GetSwapchain() {
	return m_swapchain;
}

std::vector<VkImage> Window::GetSwapchainImages() {
	return m_swapchain_images;
}

std::vector<VkImageView> Window::GetSwapchainImageViews() {
	return m_swapchain_image_views;
}

VkSurfaceFormatKHR & Window::GetSurfaceFormatKHR() {
	return m_surface_format;
}

void Window::InitSurface() {
	InitOSSurface();

	VkPhysicalDevice gpu = m_renderer->GetVulkanPhysicalDevice();

	VkBool32 WSI_supported = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(gpu, m_renderer->GetVulkanGraphicsQueueFamilyIndex(), m_surface, &WSI_supported);

	if (!WSI_supported) {
		assert(0 && "WSI not supported");
		std::exit(-1);
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &m_surface_capabilities);
	if (m_surface_capabilities.currentExtent.width < UINT32_MAX) {
		m_surface_size_x = m_surface_capabilities.currentExtent.width;
		m_surface_size_y = m_surface_capabilities.currentExtent.height;
	}
	{
		uint32_t format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &format_count, nullptr);
		if (format_count == 0) {
			assert(0 && "Surface formats missing");
			std::exit(-1);
		}
		std::vector<VkSurfaceFormatKHR> formats(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &format_count, formats.data());
		if (formats[0].format == VK_FORMAT_UNDEFINED) {
			m_surface_format.format = VK_FORMAT_B8G8R8_UNORM;
			m_surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}
		else {
			m_surface_format = formats[0];
		}
	}
}

void Window::DeInitSurface() {
	vkDestroySurfaceKHR(m_renderer->GetVulkanInstance(), m_surface, nullptr);
}

void Window::InitSwapchain() {
	if (m_swapchain_image_count > m_surface_capabilities.maxImageCount) {
		m_swapchain_image_count = m_surface_capabilities.maxImageCount;
	}
	if (m_swapchain_image_count < m_surface_capabilities.minImageCount + 1) {
		m_swapchain_image_count = m_surface_capabilities.minImageCount + 1;
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	{
		uint32_t present_mode_count = 0;
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_renderer->GetVulkanPhysicalDevice(), m_surface, &present_mode_count, nullptr));
		std::vector<VkPresentModeKHR> present_modes(present_mode_count);
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_renderer->GetVulkanPhysicalDevice(), m_surface, &present_mode_count, present_modes.data()));
		std::vector<VkPresentModeKHR>::iterator iter;
		for (iter = present_modes.begin(); iter != present_modes.end(); ++iter) {
			if (*iter == VK_PRESENT_MODE_MAILBOX_KHR) {
				present_mode = *iter;
			}
		}
	}

	VkSwapchainCreateInfoKHR swapchain_create_info{};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = m_surface;
	swapchain_create_info.minImageCount = m_swapchain_image_count;
	swapchain_create_info.imageFormat = m_surface_format.format;
	swapchain_create_info.imageColorSpace = m_surface_format.colorSpace;
	swapchain_create_info.imageExtent.width = m_surface_size_x;
	swapchain_create_info.imageExtent.height = m_surface_size_y;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.queueFamilyIndexCount = 0;
	swapchain_create_info.pQueueFamilyIndices = nullptr;
	swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

	ErrorCheck(vkCreateSwapchainKHR(m_renderer->GetVulkanDevice(), &swapchain_create_info, VK_NULL_HANDLE, &m_swapchain));

	ErrorCheck(vkGetSwapchainImagesKHR(m_renderer->GetVulkanDevice(), m_swapchain, &m_swapchain_image_count, nullptr));
}

void Window::DeInitSwapchain() {
	vkDestroySwapchainKHR(m_renderer->GetVulkanDevice(), m_swapchain, nullptr);
}

void Window::InitSwapchainImages() {
	m_swapchain_images.resize(m_swapchain_image_count);
	m_swapchain_image_views.resize(m_swapchain_image_count);

	ErrorCheck(vkGetSwapchainImagesKHR(m_renderer->GetVulkanDevice(), m_swapchain, &m_swapchain_image_count, m_swapchain_images.data()));
	for (uint32_t i = 0; i < m_swapchain_image_count; i++) {
		VkImageViewCreateInfo image_view_create_info{};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.image = m_swapchain_images[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = m_surface_format.format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		ErrorCheck(vkCreateImageView(m_renderer->GetVulkanDevice(), &image_view_create_info, nullptr, &m_swapchain_image_views[i]));
	}
}

void Window::DeInitSwapchainImages() {
	std::vector<VkImageView>::iterator iter;
	for (iter = m_swapchain_image_views.begin(); iter != m_swapchain_image_views.end(); ++iter) {
		vkDestroyImageView(m_renderer->GetVulkanDevice(), *iter, nullptr);
	}
}

void Window::InitDepthBuffer() {
	VkImageCreateInfo image_create_info{};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext = VK_NULL_HANDLE;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = VK_FORMAT_D16_UNORM;
	image_create_info.extent.width = m_surface_size_x;
	image_create_info.extent.height = m_surface_size_y;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_create_info.queueFamilyIndexCount = 0;
	image_create_info.pQueueFamilyIndices = nullptr;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.flags = 0;

	vkCreateImage(m_renderer->GetVulkanDevice(), &image_create_info, VK_NULL_HANDLE, &m_image);

	VkMemoryRequirements memreqs;
	vkGetImageMemoryRequirements(m_renderer->GetVulkanDevice(), m_image, &memreqs);
	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = VK_NULL_HANDLE;
	memory_allocate_info.allocationSize = memreqs.size;
	memory_allocate_info.memoryTypeIndex = memory_types_from_properties(memreqs.memoryTypeBits, 0, &memory_allocate_info.memoryTypeIndex, m_renderer->GetPhysicalDeviceMemoryProperties());

	ErrorCheck(vkAllocateMemory(m_renderer->GetVulkanDevice(), &memory_allocate_info, nullptr, &m_depth_buffer_memory));

	ErrorCheck(vkBindImageMemory(m_renderer->GetVulkanDevice(), m_image, m_depth_buffer_memory, 0));

	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.flags = VK_NULL_HANDLE;
	image_view_create_info.image = m_image;
	image_view_create_info.format = VK_FORMAT_D16_UNORM;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.flags = 0;

	ErrorCheck(vkCreateImageView(m_renderer->GetVulkanDevice(), &image_view_create_info, nullptr, &m_image_view));
}

void Window::DeInitDepthBuffer() {
	vkDestroyImageView(m_renderer->GetVulkanDevice(), m_image_view, nullptr);
	vkDestroyImage(m_renderer->GetVulkanDevice(), m_image, nullptr);
	vkFreeMemory(m_renderer->GetVulkanDevice(), m_depth_buffer_memory, nullptr);
}