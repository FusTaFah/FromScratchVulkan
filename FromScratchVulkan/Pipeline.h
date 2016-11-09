#pragma once

#include "Shared.h"
#include "Platform.h"
#include <vector>

#define NUM_DESCRIPTOR_SETS 1

class Pipeline {
public:
	Pipeline(const VkDevice & device, const VkPhysicalDeviceMemoryProperties & physical_device_memory_properties);
	~Pipeline();
private:
	//methods
	void InitUniformBuffer();
	void InitPipeline();

	void DeInitPipeline();

	//variables
	const VkDevice & m_device;

	const VkPhysicalDeviceMemoryProperties & m_physical_device_memory_properties;

	VkBuffer m_buffer;

	VkDeviceMemory m_uniform_buffer_memory;

	VkDescriptorBufferInfo m_buffer_info;

	std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorPool m_descriptor_pool;
};