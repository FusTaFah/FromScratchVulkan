#pragma once

#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR 1
#include <Windows.h>

#else
#error Platform not yet supported.

#endif

#include <vulkan\vulkan.h>