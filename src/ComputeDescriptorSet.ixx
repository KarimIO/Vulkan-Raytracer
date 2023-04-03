export module ComputeDescriptorSet;

import std.core;
import <vulkan/vulkan.h>;
import RaytracerTargetImage;
import Sampler;
import DescriptorPool;
import UniformBuffer;
import VulkanCore;

export class ComputeDescriptorSet {
public:
	void Initialize(RaytracerTargetImage& texture, Sampler& sampler, UniformBuffer& uniformBuffer, DescriptorPool& descriptorPool) {
		VkDevice device = VulkanCore::GetDevice();

		VkDescriptorSetLayoutBinding imageLayoutBinding{};
		imageLayoutBinding.binding = 0;
		imageLayoutBinding.descriptorCount = 1;
		imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		imageLayoutBinding.pImmutableSamplers = nullptr;
		imageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 1;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { imageLayoutBinding, uboLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool.GetDescriptorPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.imageView = texture.GetImageView();
		imageInfo.sampler = sampler.GetSampler();

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer.GetUniformBuffer(0);
		bufferInfo.offset = 0;
		bufferInfo.range = uniformBuffer.GetSize();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	VkDescriptorSet GetDescriptorSet() {
		return descriptorSet;
	}

	VkDescriptorSetLayout GetLayout() {
		return descriptorSetLayout;
	}

	~ComputeDescriptorSet() {
		VkDevice device = VulkanCore::GetDevice();

		if (descriptorSetLayout != nullptr) {
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		}
	}
private:
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
};
