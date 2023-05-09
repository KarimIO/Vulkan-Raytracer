export module DebugWindow;

import std.core;
import <imgui/imgui.h>;
import <imgui/backends/imgui_impl_glfw.h>;
import <imgui/backends/imgui_impl_vulkan.h>;
import <vulkan/vulkan.h>;
import Settings;
import VulkanCore;

export class DebugWindow {
public:
	bool Initialize(Settings* settings, std::function<void()> OnUpdateParameter) {
		this->settings = settings;
		this->OnUpdateParameter = OnUpdateParameter;

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

	~DebugWindow() {
		VkDevice device = VulkanCore::GetDevice();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(device, imguiPool, nullptr);
	}

	void SetupFrame() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void Update() {
		SetupFrame();

		ImGui::Begin("Debug Panel", &isVisible);

		RenderSceneComboBox();
		RenderRaySettings();
		RenderSkySettings();

		ImGui::End();

		ImGui::Render();
	}

	void RenderSceneComboBox() {
		const char* items[] = { "Scene 1", "Scene 2" };
		static int currentItemIndex = 0;
		const char* comboText = items[currentItemIndex];
		if (ImGui::BeginCombo("Current Scene", comboText, 0)) {
			for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
				const bool isSelected = (currentItemIndex == n);
				if (ImGui::Selectable(items[n], isSelected))
					currentItemIndex = n;

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	void RenderRaySettings() {
		if (ImGui::TreeNode("Ray Properties")) {
			if (ImGui::SliderInt("Maximum Ray Bounces", &settings->numBounces, 0, 30)) {
				OnUpdateParameter();
			}

			ImGui::SliderInt("Rays Per Pixel When Moving", &settings->numRaysWhileMoving, 1, 50);
			ImGui::SliderInt("Rays Per Pixel When Still", &settings->numRaysWhileStill, 1, 200);

			ImGui::TreePop();
		}
	}

	void RenderSkySettings() {
		if (ImGui::TreeNode("Sun & Sky")) {
			if (ImGui::SliderAngle("Angle Pitch", &settings->sunLightPitch)) {
				OnUpdateParameter();
			}

			if (ImGui::SliderAngle("Angle Yaw", &settings->sunLightYaw)) {
				OnUpdateParameter();
			}

			if (ImGui::ColorEdit3("Sky Color", &settings->skyColor[0])) {
				OnUpdateParameter();
			}

			if (ImGui::ColorEdit3("Horizon Color", &settings->horizonColor[0])) {
				OnUpdateParameter();
			}

			if (ImGui::ColorEdit3("Ground Color", &settings->groundColor[0])) {
				OnUpdateParameter();
			}

			if (ImGui::SliderFloat("Sun Focus", &settings->sunFocus, 1, 500)) {
				OnUpdateParameter();
			}

			if (ImGui::SliderFloat("Sun Intensity", &settings->sunIntensity, 1, 50)) {
				OnUpdateParameter();
			}

			ImGui::TreePop();
		}
	}

	void Render(VkCommandBuffer commandBuffer) {
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

private:
	std::function<void()> OnUpdateParameter;
	VkDescriptorPool imguiPool;
	Settings* settings;
	bool isVisible = true;
};
