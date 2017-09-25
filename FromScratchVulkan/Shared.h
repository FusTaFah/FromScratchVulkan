#pragma once

#include "Platform.h"
#include <assert.h>
#include <iostream>
#include <vector>


void ErrorCheck(VkResult result);
bool memory_types_from_properties(uint32_t type_bits, VkFlags requirements_mask, uint32_t * typeIndex, VkPhysicalDeviceMemoryProperties memory_properties);
bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv);
EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
void init_resources(TBuiltInResource &Resources);

struct vertex_data {
	glm::vec3 position;
	glm::vec3 colour;
	vertex_data(glm::vec3 pos, glm::vec3 col) : position(pos), colour(col) {}
};

struct vertex_uv_data {
	glm::vec3 position;
	glm::vec2 uv;
	vertex_uv_data(glm::vec3 pos, glm::vec2 uV) : position(pos), uv(uV) {}
};