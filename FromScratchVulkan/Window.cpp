#include "Window.h"
#include "Renderer.h"
#include <assert.h>

Window::Window(Renderer * renderer, uint32_t size_x, uint32_t size_y, std::string name) :
	m_surface_size_x(size_x),
	m_surface_size_y(size_y),
	m_window_name(name),
	m_renderer(renderer),
	m_surface(VK_NULL_HANDLE),
	m_surface_capabilities({}),
	m_surface_format({})
{
	InitOSWindow();
	InitSurface();
}

Window::~Window() {
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