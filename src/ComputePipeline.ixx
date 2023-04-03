export module ComputePipeline;

import std.core;

import <vulkan/vulkan.h>;

import ReadFile;
import ComputeDescriptorSet;
import VulkanCore;

export class ComputePipeline {
public:
	void Initialize(std::string_view path, ComputeDescriptorSet& computeDescriptorSet) {
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

		VkDescriptorSetLayout computeDescriptorSetLayout = computeDescriptorSet.GetLayout();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline layout!");
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.stage = computeShaderStageInfo;

		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline!");
		}

		vkDestroyShaderModule(device, computeShaderModule, nullptr);
	}

	VkPipeline GetPipeline() {
		return pipeline;
	}

	VkPipelineLayout GetPipelineLayout() {
		return pipelineLayout;
	}

	~ComputePipeline() {
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
