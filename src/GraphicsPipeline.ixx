export module GraphicsPipeline;

import std.core;
import ReadFile;
import <vulkan/vulkan.h>;
import VulkanCore;


export class GraphicsPipeline {
public:
	GraphicsPipeline() = default;

	GraphicsPipeline(GraphicsPipeline&& other) {
		this->pipeline = other.pipeline;
		this->pipelineLayout = other.pipelineLayout;
	}

	GraphicsPipeline(VkPipelineLayout pipelineLayout, VkPipeline pipeline) {
		Initialize(pipelineLayout, pipeline);
	}

	void Initialize(VkPipelineLayout pipelineLayout, VkPipeline pipeline) {
		this->pipeline = pipeline;
		this->pipelineLayout = pipelineLayout;
	}

	VkPipeline GetPipeline() {
		return pipeline;
	}

	VkPipelineLayout GetPipelineLayout() {
		return pipelineLayout;
	}

	~GraphicsPipeline() {
		VkDevice device = VulkanCore::GetDevice();
		if (pipeline != nullptr) {
			vkDestroyPipeline(device, pipeline, nullptr);
		}

		if (pipelineLayout != nullptr) {
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		}
	}
private:
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
};

export class GraphicsPipelineBuilder {
public:
	GraphicsPipelineBuilder(VkRenderPass renderPass, int numStages) : renderPass(renderPass) {
		shaderModules.reserve(numStages);
	}

	GraphicsPipelineBuilder& WithVertexModule(std::string_view path) {
		auto vertShaderCode = ReadFile(path.data());

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		shaderStages.push_back(vertShaderStageInfo);
		shaderModules.push_back(vertShaderModule);

		return *this;
	}

	GraphicsPipelineBuilder& WithFragmentModule(std::string_view path) {
		auto fragShaderCode = ReadFile(path.data());
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		shaderStages.push_back(fragShaderStageInfo);
		shaderModules.push_back(fragShaderModule);

		return *this;
	}

	GraphicsPipelineBuilder& WithVertexBindings(VkVertexInputBindingDescription* bindingPtr, uint32_t bindingCount) {
		vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
		vertexInputInfo.pVertexBindingDescriptions = bindingPtr;

		return *this;
	}

	GraphicsPipelineBuilder& WithVertexAttributes(VkVertexInputAttributeDescription* attribPtr, uint32_t attribCount) {
		vertexInputInfo.vertexAttributeDescriptionCount = attribCount;
		vertexInputInfo.pVertexAttributeDescriptions = attribPtr;

		return *this;
	}

	GraphicsPipelineBuilder& WithDescriptors(VkDescriptorSetLayout* descriptorSetLayouts, uint32_t layoutCount) {
		pipelineLayoutInfo.setLayoutCount = layoutCount;
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;

		return *this;
	}

	void Build(GraphicsPipeline& graphicsPipeline) {
		VkDevice device = VulkanCore::GetDevice();

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		VkPipelineLayout pipelineLayout;
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		for (size_t i = 0; i < shaderModules.size(); ++i) {
			vkDestroyShaderModule(device, shaderModules[i], nullptr);
		}

		graphicsPipeline.Initialize(pipelineLayout, pipeline);
	}
private:
	VkShaderModule CreateShaderModule(const std::vector<char>& code) {
		VkDevice device = VulkanCore::GetDevice();

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	VkRenderPass renderPass = nullptr;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkShaderModule> shaderModules;
};
