#include "Pipeline.h"

Pipeline::Pipeline(const VkDevice & device, const VkDescriptorBufferInfo & uniform_buffer_info) :
	m_device(device),
	m_uniform_buffer_info(uniform_buffer_info)
{
	InitPipeline();
}

Pipeline::~Pipeline() {
	DeInitPipeline();
}

void Pipeline::InitPipeline() {
	VkDescriptorSetLayoutBinding descriptor_set_layout_binding{};
	descriptor_set_layout_binding.binding = 0;
	descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_set_layout_binding.descriptorCount = 1;
	descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
	descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.pNext = VK_NULL_HANDLE;
	descriptor_set_layout_create_info.bindingCount = 1;
	descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;

	m_descriptor_set_layouts.resize(NUM_DESCRIPTOR_SETS);

	ErrorCheck(vkCreateDescriptorSetLayout(m_device, &descriptor_set_layout_create_info, VK_NULL_HANDLE, m_descriptor_set_layouts.data()));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.pNext = VK_NULL_HANDLE;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = VK_NULL_HANDLE;
	pipeline_layout_create_info.setLayoutCount = NUM_DESCRIPTOR_SETS;
	pipeline_layout_create_info.pSetLayouts = m_descriptor_set_layouts.data();

	ErrorCheck(vkCreatePipelineLayout(m_device, &pipeline_layout_create_info, VK_NULL_HANDLE, &m_pipeline_layout));

	VkDescriptorPoolSize descriptor_pool_size[1];
	descriptor_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_pool_size[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.pNext = VK_NULL_HANDLE;
	descriptor_pool_create_info.maxSets = 1;
	descriptor_pool_create_info.poolSizeCount = 1;
	descriptor_pool_create_info.pPoolSizes = descriptor_pool_size;

	ErrorCheck(vkCreateDescriptorPool(m_device, &descriptor_pool_create_info, VK_NULL_HANDLE, &m_descriptor_pool));

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info[1];
	descriptor_set_allocate_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info[0].pNext = VK_NULL_HANDLE;
	descriptor_set_allocate_info[0].descriptorPool = m_descriptor_pool;
	descriptor_set_allocate_info[0].descriptorSetCount = 1;
	descriptor_set_allocate_info[0].pSetLayouts = m_descriptor_set_layouts.data();
	m_descriptor_set_layouts.resize(1);

	std::vector<VkDescriptorSet> descriptor_sets;
	descriptor_sets.resize(NUM_DESCRIPTOR_SETS);
	vkAllocateDescriptorSets(m_device, descriptor_set_allocate_info, descriptor_sets.data());

	VkWriteDescriptorSet write_descriptor_set[1];
	write_descriptor_set[0] = {};
	write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[0].pNext = VK_NULL_HANDLE;
	write_descriptor_set[0].dstSet = descriptor_sets[0];
	write_descriptor_set[0].descriptorCount = 1;
	write_descriptor_set[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_set[0].pBufferInfo = &m_uniform_buffer_info;
	write_descriptor_set[0].dstArrayElement = 0;
	write_descriptor_set[0].dstBinding = 0;

	vkUpdateDescriptorSets(m_device, 1, write_descriptor_set, 0, VK_NULL_HANDLE);
}

void Pipeline::DeInitPipeline() {
	vkDestroyDescriptorPool(m_device, m_descriptor_pool, VK_NULL_HANDLE);
	vkDestroyBuffer(m_device, m_uniform_buffer_info.buffer, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layouts[0], VK_NULL_HANDLE);
	//vkDestroyPipelineLayout(m_device, m_pipeline_layout, VK_NULL_HANDLE);
}