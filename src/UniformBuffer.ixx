export module UniformBuffer;

import std.core;
import <glm/glm.hpp>;
import <vulkan/vulkan.h>;
import VulkanCore;

export class UniformBuffer {
	size_t uniformBufferCount;
	uint32_t bufferSize;

public:
	void Initialize(size_t uniformBufferCount, VkBufferUsageFlags usage, uint32_t bufferSize) {
		this->uniformBufferCount = uniformBufferCount;
		this->bufferSize = bufferSize;

		VkDevice device = VulkanCore::GetDevice();
		VkDeviceSize uboSize = bufferSize;

		uniformBuffers.resize(uniformBufferCount);
		uniformBuffersMemory.resize(uniformBufferCount);
		uniformBuffersMapped.resize(uniformBufferCount);

		for (size_t i = 0; i < uniformBufferCount; i++) {
			VulkanCore::CreateBuffer(uboSize, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void* GetMappedBuffer(size_t i) {
		return uniformBuffersMapped[i];
	}

	template<typename T>
	T& GetMappedBuffer(size_t i) {
		return *static_cast<T*>(uniformBuffersMapped[i]);
	}

	void* GetMappedBuffer() {
		return uniformBuffersMapped[0];
	}

	template<typename T>
	T& GetMappedBuffer() {
		return *static_cast<T*>(uniformBuffersMapped[0]);
	}

	uint32_t GetSize() {
		return bufferSize;
	}

	VkBuffer GetUniformBuffer(size_t i) {
		return uniformBuffers[i];
	}

	VkBuffer GetUniformBuffer() {
		return uniformBuffers[0];
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
