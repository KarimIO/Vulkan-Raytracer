export module ComputeDescriptorSet;

import std.core;
import <vulkan/vulkan.h>;
import RaytracerTargetImage;
import Sampler;
import DescriptorPool;
import VulkanCore;

export class ComputeDescriptorSet {
public:
	void Initialize(RaytracerTargetImage& texture, Sampler& sampler, DescriptorPool& descriptorPool) {
		VkDevice device = VulkanCore::GetDevice();

		VkDescriptorSetLayoutBinding imageLayoutBinding{};
		imageLayoutBinding.binding = 0;
		imageLayoutBinding.descriptorCount = 1;
		imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		imageLayoutBinding.pImmutableSamplers = nullptr;
		imageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = { imageLayoutBinding };
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

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
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
