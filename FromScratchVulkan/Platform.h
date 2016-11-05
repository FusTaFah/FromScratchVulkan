#pragma once

#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR 1
#define	PLATFORM_SURFACE_EXTENTION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#include <Windows.h>

#elif defined(__linux)

#define VK_USE_PLATFORM_XCB_KHR 1
#define	PLATFORM_SURFACE_EXTENTION_NAME VK_KHR_XCB_SURFACE_EXTENTION_NAME
#include <xcb/xcb.h>

#else
#error Platform not yet supported.

#endif

#include <vulkan\vulkan.h>
#include <glm\glm.hpp>
#include <glm\matrix.hpp>
#include <glm\mat4x4.hpp>
#include <glm\vec2.hpp>
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm\gtc\matrix_transform.hpp>