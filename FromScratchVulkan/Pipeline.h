#pragma once

#include "Shared.h"
#include "Platform.h"
#include <vector>

#define NUM_DESCRIPTOR_SETS 1

class Pipeline {
public:
	Pipeline(const VkDevice & device, const VkDescriptorBufferInfo & uniform_buffer_info);
	~Pipeline();
private:
	//methods
	void InitPipeline();
	void DeInitPipeline();

	//variables
	const VkDevice & m_device;
	const VkDescriptorBufferInfo & m_uniform_buffer_info;
	std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorPool m_descriptor_pool;
};