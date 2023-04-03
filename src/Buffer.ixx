export module Buffer;

import std.core;
import <vulkan/vulkan.h>;
import VulkanCore;

export class Buffer {
public:
	void Initialize(const void* srcData, size_t size, VkBufferUsageFlagBits usageBit) {
		VkDevice device = VulkanCore::GetDevice();

		VkDeviceSize bufferSize = size;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VulkanCore::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* dstData;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &dstData);
		memcpy(dstData, srcData, bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		VulkanCore::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageBit, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);

		VulkanCore::CopyBuffer(stagingBuffer, buffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	~Buffer() {
		VkDevice device = VulkanCore::GetDevice();

		if (buffer != nullptr) {
			vkDestroyBuffer(device, buffer, nullptr);
		}

		if (bufferMemory != nullptr) {
			vkFreeMemory(device, bufferMemory, nullptr);
		}
	}
private:
	VkBuffer buffer = nullptr;
	VkDeviceMemory bufferMemory = nullptr;
};
