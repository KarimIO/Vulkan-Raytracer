export module RaytracerTargetImage;

import std.core;
import <vulkan/vulkan.h>;
import <stb_image.h>;
import VulkanCore;

export class RaytracerTargetImage {
public:
	void Initialize(int width, int height) {
		VkDevice device = VulkanCore::GetDevice();

		VulkanCore::CreateImage(
			width,
			height,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			image,
			imageMemory
		);
		imageView = VulkanCore::CreateImageView(image, VK_FORMAT_R8G8B8A8_UNORM);
	}

	VkImageView GetImageView() {
		return imageView;
	}

	~RaytracerTargetImage() {
		VkDevice device = VulkanCore::GetDevice();

		if (imageView != nullptr) {
			vkDestroyImageView(device, imageView, nullptr);
			imageView = nullptr;
		}

		if (image != nullptr) {
			vkDestroyImage(device, image, nullptr);
			image = nullptr;
		}

		if (imageMemory != nullptr) {
			vkFreeMemory(device, imageMemory, nullptr);
			imageMemory = nullptr;
		}
	}
private:
	VkImage image = nullptr;
	VkImageView imageView = nullptr;
	VkDeviceMemory imageMemory = nullptr;
};