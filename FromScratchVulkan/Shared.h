#pragma once

#include "Platform.h"
#include <assert.h>
#include <iostream>

void ErrorCheck(VkResult result);
bool memory_types_from_properties(uint32_t type_bits, VkFlags requirements_mask, uint32_t * typeIndex, VkPhysicalDeviceMemoryProperties memory_properties);