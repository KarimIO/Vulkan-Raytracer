export module UniformBuffer;

import std.core;
import <glm/glm.hpp>;
import <vulkan/vulkan.h>;
import VulkanCore;

struct UniformBufferObject {
	alignas(16) glm::mat4 cameraToWorld;
	alignas(16) glm::mat4 cameraInverseProj;
	float time;
	int maxRayBounceCount;
	int numRaysPerPixel;
};

export class UniformBuffer {
	const int MAX_FRAMES_IN_FLIGHT = 2;
public:
	void Initialize() {
		VkDevice device = VulkanCore::GetDevice();
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VulkanCore::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void SetUniformData(double time, glm::mat4& cameraToWorld, glm::mat4& cameraInverseProj) {
		uint32_t currentFrame = VulkanCore::GetVulkanCoreInstance().GetCurrentFrame();

		UniformBufferObject ubo{};
		ubo.cameraToWorld = cameraToWorld;
		ubo.cameraInverseProj = cameraInverseProj;
		ubo.time = static_cast<float>(time);
		ubo.maxRayBounceCount = 2;
		ubo.numRaysPerPixel = 50;

		memcpy(uniformBuffersMapped[0], &ubo, sizeof(ubo));
	}

	uint32_t GetSize() {
		return sizeof(UniformBufferObject);
	}

	VkBuffer GetUniformBuffer(uint32_t i) {
		return uniformBuffers[i];
	}

	VkBuffer GetCurrentUniformBuffer() {
		uint32_t currentFrame = VulkanCore::GetVulkanCoreInstance().GetCurrentFrame();
		return uniformBuffers[currentFrame];
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