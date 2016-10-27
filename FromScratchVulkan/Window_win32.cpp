#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Window.h"
#include "Renderer.h"
#include <assert.h>

#ifdef VK_USE_PLATFORM_WIN32_KHR

LRESULT CALLBACK WindowsEventHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Window * window = reinterpret_cast<Window *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (uMsg) {
	case WM_CLOSE:
		window->Close();
		return 0;
	case WM_SIZE:
		//if the window is resized then we should rebuild some things
		//but by default we leave this blank
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

uint64_t Window::m_win32_class_id_counter = 0;

void Window::InitOSWindow() {
	WNDCLASSEX win_class{};
	assert(m_surface_size_x > 0);
	assert(m_surface_size_y > 0);

	m_win32_instance = GetModuleHandle(nullptr);
	m_win32_class_name = m_window_name + "_" + std::to_string(m_win32_class_id_counter);
	m_win32_class_id_counter++;

	//Initialize the window class structure:
	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WindowsEventHandler;
	win_class.cbClsExtra = 0;
	win_class.cbWndExtra = 0;
	win_class.hInstance = m_win32_instance;
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = m_win32_class_name.c_str();
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	//Register window class:
	if (!RegisterClassEx(&win_class)) {
		//It didn't work, so try to give a useful error
		assert(0 && "Cannot create a window in which to draw!\n");
		fflush(stdout);
		std::exit(-1);
	}
	DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	//create window with registered class
	RECT wr = { 0, 0, LONG(m_surface_size_x), LONG(m_surface_size_y) };
	AdjustWindowRectEx(&wr, style, FALSE, ex_style);
	m_win32_window = CreateWindow(
		m_win32_class_name.c_str(),
		m_window_name.c_str(),
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wr.right - wr.left,
		wr.bottom - wr.top,
		NULL,
		NULL,
		m_win32_instance,
		NULL
	);
	if (!m_win32_window) {
		//it didn't work so try to give a useful error:
		assert(0 && "Cannot create a window in which to draw!\n");
		fflush(stdout);
		std::exit(-1);
	}
	SetWindowLongPtr(m_win32_window, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(m_win32_window, SW_SHOW);
	SetForegroundWindow(m_win32_window);
	SetFocus(m_win32_window);
}

void Window::DeInitOSWindow() {
	DestroyWindow(m_win32_window);
	UnregisterClass(m_win32_class_name.c_str(), m_win32_instance);
}

void Window::UpdateOSWindow() {
	MSG msg;
	if (PeekMessage(&msg, m_win32_window, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Window::InitOSSurface() {

	vkCreateWin32SurfaceKHR(m_renderer->GetVulkanInstance(), , nullptr);
}

#endif