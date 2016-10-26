#include "Window.h"

Window::Window(uint32_t size_x, uint32_t size_y, std::string name):
	m_surface_size_x(size_x),
	m_surface_size_y(size_y),
	m_window_name(name)
{
	InitOSWindow();
}

Window::~Window() {
	DeInitOSWindow();
}

void Window::Close() {
	m_running = false;
}

bool Window::Update() {
	UpdateOSWindow();
	return m_running;
}