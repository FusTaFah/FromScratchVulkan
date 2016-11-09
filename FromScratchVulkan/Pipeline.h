#pragma once

#include "Shared.h"
#include "Platform.h"
#include <vector>

#define NUM_DESCRIPTOR_SETS 1

class Window;
class Renderer;

class Pipeline {
public:
	Pipeline(Renderer * renderer);
	~Pipeline();
private:
	//methods
	void InitUniformBuffer();
	void InitPipeline();

	void DeInitPipeline();

	//variables
	Renderer * m_renderer;

	VkBuffer m_buffer;

	VkDeviceMemory m_uniform_buffer_memory;

	VkDescriptorBufferInfo m_buffer_info;

	std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorPool m_descriptor_pool;
};