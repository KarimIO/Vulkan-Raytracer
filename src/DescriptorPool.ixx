export module DescriptorPool;

import std.core;
import <vulkan/vulkan.h>;
import VulkanCore;

export class DescriptorPool {
public:
	void Initialize(VkDescriptorPool descriptorPool) {
		this->descriptorPool = descriptorPool;
	}

	VkDescriptorPool GetDescriptorPool() {
		return descriptorPool;
	}

	~DescriptorPool() {
		VkDevice device = VulkanCore::GetDevice();

		if (descriptorPool != nullptr) {
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		}
	}
private:
	VkDescriptorPool descriptorPool = nullptr;
};

export class DescriptorPoolBuilder {
public:
	DescriptorPoolBuilder& WithUniformBuffers(uint32_t count) {
		poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);

		return *this;
	}

	DescriptorPoolBuilder& WithStorageImages(uint32_t count) {
		poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count);

		return *this;
	}

	DescriptorPoolBuilder& WithImageSamplers(uint32_t count) {
		poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count);

		return *this;
	}

	void Build(DescriptorPool& descriptorPoolContainer) {
		VkDevice device = VulkanCore::GetDevice();

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(4);

		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

		descriptorPoolContainer.Initialize(descriptorPool);
	}

private:
	std::vector<VkDescriptorPoolSize> poolSizes;
};
