export module ComputePipeline;

import std.core;
import ReadFile;
import <vulkan/vulkan.h>;
import VulkanCore;

export class ComputePipeline {
public:
	void Initialize(std::string_view path) {
		VkDevice device = VulkanCore::GetDevice();
		auto computeShaderCode = ReadFile(path.data());

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = computeShaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(computeShaderCode.data());

		VkShaderModule computeShaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &computeShaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = computeShaderModule;
		computeShaderStageInfo.pName = "main";
		/*
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline layout!");
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = computePipelineLayout;
		pipelineInfo.stage = computeShaderStageInfo;

		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline!");
		}

		vkDestroyShaderModule(device, computeShaderModule, nullptr);
		*/
	}

	~ComputePipeline() {
		if (pipeline != nullptr) {
			VkDevice device = VulkanCore::GetDevice();
			vkDestroyPipeline(device, pipeline, nullptr);
		}
	}
private:
	VkPipeline pipeline = nullptr;
};
