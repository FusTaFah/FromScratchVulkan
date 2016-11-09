#include "Pipeline.h"

Pipeline::Pipeline(const VkDevice & device, const VkPhysicalDeviceMemoryProperties & physical_device_memory_properties) :
	m_device(device),
	m_physical_device_memory_properties(physical_device_memory_properties)
{
	InitUniformBuffer();
	InitPipeline();
}

Pipeline::~Pipeline() {
	DeInitPipeline();
}

void Pipeline::InitUniformBuffer() {
	glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	glm::mat4 view_matrix = glm::lookAt(
		glm::vec3(0.0f, 3.0f, 10.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, -1.0f, 0.0f)
	);
	glm::mat4 model_matrix = glm::mat4(1.0f);
	glm::mat4 clip_matrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	);
	glm::mat4 model_view_projection_matrix = clip_matrix * projection_matrix * view_matrix * model_matrix;

	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = VK_NULL_HANDLE;
	buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buffer_create_info.size = sizeof(model_view_projection_matrix);
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = VK_NULL_HANDLE;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.flags = 0;
	ErrorCheck(vkCreateBuffer(m_device, &buffer_create_info, VK_NULL_HANDLE, &m_buffer));

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(m_device, m_buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = VK_NULL_HANDLE;
	memory_allocate_info.memoryTypeIndex = 0;
	memory_allocate_info.allocationSize = memory_requirements.size;

	if (!memory_types_from_properties(memory_requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&memory_allocate_info.memoryTypeIndex,
		m_physical_device_memory_properties)) {

		assert(0 && "memory assignment error");
	}
	else {
		ErrorCheck(vkAllocateMemory(m_device, &memory_allocate_info, VK_NULL_HANDLE, &m_uniform_buffer_memory));
	}

	uint32_t *pData;
	//map memory
	ErrorCheck(vkMapMemory(m_device, m_uniform_buffer_memory, 0, memory_requirements.size, 0, (void **)&pData));
	//copy model view projection matrix to uniform buffer
	memcpy(pData, &model_view_projection_matrix, sizeof(model_view_projection_matrix));
	//unmap memory
	vkUnmapMemory(m_device, m_uniform_buffer_memory);
	//bind buffer to gpu
	vkBindBufferMemory(m_device, m_buffer, m_uniform_buffer_memory, 0);

	m_buffer_info = {};
	m_buffer_info.buffer = m_buffer;
	m_buffer_info.offset = 0;
	m_buffer_info.range = sizeof(model_view_projection_matrix);
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
	descriptor_set_allocate_info[0].descriptorSetCount = NUM_DESCRIPTOR_SETS;
	descriptor_set_allocate_info[0].pSetLayouts = m_descriptor_set_layouts.data();
	m_descriptor_set_layouts.resize(1);

	std::vector<VkDescriptorSet> descriptor_sets;
	descriptor_sets.resize(NUM_DESCRIPTOR_SETS);

	ErrorCheck( vkAllocateDescriptorSets(m_device, descriptor_set_allocate_info, descriptor_sets.data()));

	VkWriteDescriptorSet write_descriptor_set[1];
	write_descriptor_set[0] = {};
	write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[0].pNext = VK_NULL_HANDLE;
	write_descriptor_set[0].dstSet = descriptor_sets[0];
	write_descriptor_set[0].descriptorCount = 1;
	write_descriptor_set[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_set[0].pBufferInfo = &m_buffer_info;
	write_descriptor_set[0].dstArrayElement = 0;
	write_descriptor_set[0].dstBinding = 0;

	vkUpdateDescriptorSets(m_device, 1, write_descriptor_set, 0, VK_NULL_HANDLE);
}

void Pipeline::DeInitPipeline() {
	vkDestroyDescriptorPool(m_device, m_descriptor_pool, NULL);
	vkDestroyBuffer(m_device, m_buffer, VK_NULL_HANDLE);
	vkFreeMemory(m_device, m_uniform_buffer_memory, nullptr);
	
	for (int i = 0; i < NUM_DESCRIPTOR_SETS; i++) {
		vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layouts[i], VK_NULL_HANDLE);
	}
	vkDestroyPipelineLayout(m_device, m_pipeline_layout, VK_NULL_HANDLE);
}