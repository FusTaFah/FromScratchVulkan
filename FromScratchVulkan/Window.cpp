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
}

Window::~Window() {
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