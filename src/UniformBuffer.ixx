export module UniformBuffer;

import std.core;
import <glm/glm.hpp>;
import <vulkan/vulkan.h>;
import VulkanCore;

export class UniformBuffer {
	size_t uniformBufferCount;
	uint32_t bufferSize;

public:
	void Initialize(size_t uniformBufferCount, uint32_t bufferSize) {
		this->uniformBufferCount = uniformBufferCount;
		this->bufferSize = bufferSize;

		VkDevice device = VulkanCore::GetDevice();
		VkDeviceSize uboSize = bufferSize;

		uniformBuffers.resize(uniformBufferCount);
		uniformBuffersMemory.resize(uniformBufferCount);
		uniformBuffersMapped.resize(uniformBufferCount);

		for (size_t i = 0; i < uniformBufferCount; i++) {
			VulkanCore::CreateBuffer(uboSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void* GetMappedBuffer(size_t i) {
		return uniformBuffersMapped[i];
	}

	uint32_t GetSize() {
		return bufferSize;
	}

	VkBuffer GetUniformBuffer(uint32_t i) {
		return uniformBuffers[i];
	}

	~UniformBuffer() {
		VkDevice device = VulkanCore::GetDevice();

		for (size_t i = 0; i < uniformBuffers.size(); i++) {
			if (uniformBuffers[i] != nullptr) {
				vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			}
		}

		for (size_t i = 0; i < uniformBuffersMemory.size(); i++) {
			if (uniformBuffersMemory[i] != nullptr) {
				vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
			}
		}
	}
private:
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
};
