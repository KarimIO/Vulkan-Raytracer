export module DebugWindow;

import std.core;
import <imgui/imgui.h>;
import <imgui/backends/imgui_impl_glfw.h>;
import <imgui/backends/imgui_impl_vulkan.h>;
import <vulkan/vulkan.h>;
import VulkanCore;

export class DebugWindow {
public:
	bool Initialize() {
		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000;
		poolInfo.poolSizeCount = std::size(poolSizes);
		poolInfo.pPoolSizes = poolSizes;

		VkDescriptorPool imguiPool;
		if (vkCreateDescriptorPool(VulkanCore::GetDevice(), &poolInfo, nullptr, &imguiPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate imgui descriptor pool!");
		}

		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForVulkan(VulkanCore::GetWindow(), true);

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = VulkanCore::GetInstance();
		initInfo.PhysicalDevice = VulkanCore::GetPhysicalDevice();
		initInfo.Device = VulkanCore::GetDevice();
		initInfo.Queue = VulkanCore::GetGraphicsQueue();
		initInfo.DescriptorPool = imguiPool;
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&initInfo, VulkanCore::GetVulkanCoreInstance().GetRenderPass());

		VkCommandBuffer commandBuffer = VulkanCore::BeginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		VulkanCore::EndSingleTimeCommands(commandBuffer);

		ImGui_ImplVulkan_DestroyFontUploadObjects();

		return true;
	}

	void SetupFrame() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void Update() {
		SetupFrame();

		ImGui::Begin("Debug Panel", &isVisible);
		if (ImGui::TreeNode("Ray Properties")) {
			static int i1 = 0;
			ImGui::SliderInt("Maximum Ray Bounces", &i1, 0, 30);

			static int i2 = 0;
			ImGui::SliderInt("Rays Per Pixel When Moving", &i2, 1, 50);

			static int i3 = 0;
			ImGui::SliderInt("Rays Per Pixel When Still", &i3, 1, 200);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Sun & Sky")) {
			static float angle = 0.0f;
			ImGui::SliderAngle("Angle Pitch", &angle);
			ImGui::SliderAngle("Angle Yaw", &angle);

			static float col1[3] = { 255.0f, 0.0f, 60.0f };
			ImGui::ColorEdit3("Sky Color", col1);

			static float col2[3] = { 255.0f, 0.0f, 60.0f };
			ImGui::ColorEdit3("Horizon Color", col2);

			static float col3[3] = { 255.0f, 0.0f, 60.0f };
			ImGui::ColorEdit3("Ground Color", col3);

			static float f1 = 0;
			ImGui::SliderFloat("Sun Focus", &f1, 1, 500);

			static float f2 = 0;
			ImGui::SliderFloat("Sun Intensity", &f2, 1, 50);
			ImGui::TreePop();
		}
		ImGui::End();

		ImGui::Render();
	}

	void Render(VkCommandBuffer commandBuffer) {
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

private:
	bool isVisible = true;
};
