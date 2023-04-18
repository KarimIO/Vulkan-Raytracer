export module RaytracerTargetImage;

import std.core;
import <GLFW/glfw3.h>;
import <vulkan/vulkan.h>;
import VulkanCore;

export class RaytracerTargetImage {
public:
	void Initialize() {
		int width, height;
		VulkanCore::GetVulkanCoreInstance().GetSize(width, height);
		Initialize(width, height);
	}
	
	void Initialize(int width, int height) {
		VkDevice device = VulkanCore::GetDevice();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VulkanCore::CreateImage(
				width,
				height,
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				images[i],
				imageMemories[i]
			);

			VulkanCore::TransitionImageLayout(
				images[i],
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL
			);

			imageViews[i] = VulkanCore::CreateImageView(images[i], VK_FORMAT_R8G8B8A8_UNORM);
		}
	}

	void Resize(int width, int height) {
		Cleanup();

		Initialize(width, height);
	}

	VkImageView GetImageView(size_t i) {
		return imageViews[i];
	}

	~RaytracerTargetImage() {
		Cleanup();
	}
	
	void Cleanup() {
		VkDevice device = VulkanCore::GetDevice();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			if (imageViews[i] != nullptr) {
				vkDestroyImageView(device, imageViews[i], nullptr);
				imageViews[i] = nullptr;
			}

			if (images[i] != nullptr) {
				vkDestroyImage(device, images[i], nullptr);
				images[i] = nullptr;
			}

			if (imageMemories[i] != nullptr) {
				vkFreeMemory(device, imageMemories[i], nullptr);
				imageMemories[i] = nullptr;
			}
		}
	}
private:
	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> images;
	std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> imageViews;
	std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> imageMemories;
};