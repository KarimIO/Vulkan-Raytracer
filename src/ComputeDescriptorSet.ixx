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
	void Initialize(RaytracerTargetImage& texture, Sampler& sampler, UniformBuffer& renderUniformBuffer, UniformBuffer& materialUniformBuffer, UniformBuffer& sphereUniformBuffer, DescriptorPool& descriptorPool) {
		VkDevice device = VulkanCore::GetDevice();

		VkDescriptorSetLayoutBinding imageLayoutBinding{};
		imageLayoutBinding.binding = 0;
		imageLayoutBinding.descriptorCount = 1;
		imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		imageLayoutBinding.pImmutableSamplers = nullptr;
		imageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding renderUboLayoutBinding{};
		renderUboLayoutBinding.binding = 1;
		renderUboLayoutBinding.descriptorCount = 1;
		renderUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		renderUboLayoutBinding.pImmutableSamplers = nullptr;
		renderUboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding sphereUboLayoutBinding{};
		sphereUboLayoutBinding.binding = 2;
		sphereUboLayoutBinding.descriptorCount = 1;
		sphereUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		sphereUboLayoutBinding.pImmutableSamplers = nullptr;
		sphereUboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding materialUboLayoutBinding{};
		materialUboLayoutBinding.binding = 3;
		materialUboLayoutBinding.descriptorCount = 1;
		materialUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		materialUboLayoutBinding.pImmutableSamplers = nullptr;
		materialUboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { imageLayoutBinding, renderUboLayoutBinding, sphereUboLayoutBinding, materialUboLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool.GetDescriptorPool();
		allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		allocInfo.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute descriptor sets!");
		}

		VkDescriptorBufferInfo materialBufferInfo{};
		materialBufferInfo.buffer = materialUniformBuffer.GetUniformBuffer();
		materialBufferInfo.offset = 0;
		materialBufferInfo.range = materialUniformBuffer.GetSize();

		VkDescriptorBufferInfo sphereBufferInfo{};
		sphereBufferInfo.buffer = sphereUniformBuffer.GetUniformBuffer();
		sphereBufferInfo.offset = 0;
		sphereBufferInfo.range = sphereUniformBuffer.GetSize();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageInfo.imageView = texture.GetImageView(i);
			imageInfo.sampler = sampler.GetSampler();

			VkDescriptorBufferInfo renderBufferInfo{};
			renderBufferInfo.buffer = renderUniformBuffer.GetUniformBuffer(i);
			renderBufferInfo.offset = 0;
			renderBufferInfo.range = renderUniformBuffer.GetSize();

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &imageInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &renderBufferInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &materialBufferInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &sphereBufferInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void UpdateTargetImage(RaytracerTargetImage& texture, Sampler& sampler) {
		VkDevice device = VulkanCore::GetDevice();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageInfo.imageView = texture.GetImageView(i);
			imageInfo.sampler = sampler.GetSampler();

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}
	}

	VkDescriptorSet GetDescriptorSet(size_t i) {
		return descriptorSets[i];
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
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
};
