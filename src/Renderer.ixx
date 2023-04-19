export module Renderer;

import std.core;

import <glm/glm.hpp>;
import <vulkan/vulkan.h>;

import Buffer;
import ComputePipeline;
import ComputeDescriptorSet;
import DescriptorSet;
import DescriptorPool;
import GraphicsPipeline;
import Texture;
import RaytracerTargetImage;
import VulkanCore;

struct Vertex {
	glm::vec2 pos;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> GetAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		return attributeDescriptions;
	}
};

const std::vector<Vertex> vertices = {
	{{0.0f, 0.0f}},
	{{1.0f, 0.0f}},
	{{1.0f, 1.0f}},
	{{0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 2, 1, 3, 2, 0
};

struct UniformBufferObject {
	alignas(16) glm::mat4 cameraToWorld;
	alignas(16) glm::mat4 cameraInverseProj;
	float time;
	int maxRayBounceCount;
	int numRaysPerPixel;
	alignas(16) glm::vec3 sunLightDirection;
	alignas(16) glm::vec3 skyColor = glm::vec3(0.11, 0.36, 0.57);
	alignas(16) glm::vec3 horizonColor = glm::vec3(0.83, 0.82, 0.67);
	alignas(16) glm::vec3 groundColor = glm::vec3(0.2, 0.2, 0.2);
	float sunFocus = 200.0f;
	float sunIntensity = 10.0f;
};

export class Renderer {
public:
	bool Initialize(VulkanCore* vulkanCore) {
		std::cout << "Initializing Renderer...\n";

		this->vulkanCore = vulkanCore;
		vulkanCore->PassResizeFramebufferCallback([&](int w, int h) { this->ResizeFramebufferCallback(w, h); });

		VulkanCore::GetVulkanCoreInstance().GetSize(screenWidth, screenHeight);

		raytracerTargetImage.Initialize(screenWidth, screenHeight);
		sampler.Initialize();

		vertexBuffer.Initialize(static_cast<const void*>(vertices.data()), vertices.size() * sizeof(vertices[0]), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		indexBuffer.Initialize(static_cast<const void*>(indices.data()), indices.size() * sizeof(indices[0]), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

		uniformBuffer.Initialize(2, sizeof(UniformBufferObject));

		VkVertexInputBindingDescription vertexBindingDescription = Vertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 1> vertexAttributeDescription = Vertex::GetAttributeDescriptions();

		DescriptorPoolBuilder()
			.WithStorageImages(MAX_FRAMES_IN_FLIGHT)
			.WithUniformBuffers(MAX_FRAMES_IN_FLIGHT)
			.WithImageSamplers(MAX_FRAMES_IN_FLIGHT)
			.Build(descriptorPool);

		descriptorSet.Initialize(raytracerTargetImage, sampler, descriptorPool);
		computeDescriptorSet.Initialize(raytracerTargetImage, sampler, uniformBuffer, descriptorPool);
		VkDescriptorSetLayout descriptorSetLayout = descriptorSet.GetLayout();

		computePipeline.Initialize("assets/shaders/Raytracing.comp.spv", computeDescriptorSet);

		GraphicsPipelineBuilder(vulkanCore->GetRenderPass(), 2)
			.WithVertexModule("assets/shaders/Fullscreen.vert.spv")
			.WithFragmentModule("assets/shaders/Fullscreen.frag.spv")
			.WithVertexBindings(&vertexBindingDescription, 1)
			.WithVertexAttributes(vertexAttributeDescription.data(), static_cast<uint32_t>(vertexAttributeDescription.size()))
			.WithDescriptors(&descriptorSetLayout, 1)
			.Build(graphicsPipeline);

		std::cout << "Initialized Renderer.\n";
		return true;
	}

	void ResizeFramebufferCallback(int width, int height) {
		raytracerTargetImage.Resize(width, height);
	}

	void SetUniformData(double time, glm::mat4& cameraToWorld, glm::mat4& cameraInverseProj) {
		uint32_t currentFrame = vulkanCore->GetCurrentFrame();

		UniformBufferObject ubo{};
		ubo.cameraToWorld = cameraToWorld;
		ubo.cameraInverseProj = cameraInverseProj;
		ubo.time = static_cast<float>(time);
		ubo.maxRayBounceCount = 2;
		ubo.numRaysPerPixel = 150;
		double sunSin = glm::sin(time);
		double sunCos = glm::cos(time);
		ubo.sunLightDirection = glm::normalize(glm::vec3(0.2, sunSin, sunCos));
		ubo.skyColor = glm::vec3(0.11, 0.36, 0.57);
		ubo.horizonColor = glm::vec3(0.83, 0.82, 0.67);
		ubo.groundColor = glm::vec3(0.2, 0.2, 0.2);
		ubo.sunFocus = 200.0f;
		ubo.sunIntensity = 20.0f;

		void* mappedMemory = uniformBuffer.GetMappedBuffer(currentFrame);
		memcpy(mappedMemory, &ubo, sizeof(ubo));
	}

	void Render() {
		{
			VkCommandBuffer computeCommandBuffer = vulkanCore->PrepareComputeCommandBuffer();
			if (computeCommandBuffer == nullptr) {
				return;
			}

			RecordComputeCommandBuffer(computeCommandBuffer);
			vulkanCore->SubmitComputeCommandBuffer(computeCommandBuffer);
		}

		{
			VkCommandBuffer graphicsCommandBuffer = vulkanCore->PrepareGraphicsCommandBuffer();
			if (graphicsCommandBuffer == nullptr) {
				return;
			}

			RecordCommandBuffer(graphicsCommandBuffer);
			vulkanCore->SubmitRenderCommandBuffer(graphicsCommandBuffer);
		}

		vulkanCore->PresentSwapChainImage();
	}

	void RecordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.GetPipeline());

		VkDescriptorSet descriptorSet = computeDescriptorSet.GetDescriptorSet(vulkanCore->GetCurrentFrame());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, screenWidth / 16, screenHeight / 16, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}

	void RecordCommandBuffer(VkCommandBuffer commandBuffer) {
		VkExtent2D swapChainExtent = vulkanCore->GetSwapchainExtent();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = vulkanCore->GetRenderPass();
		renderPassInfo.framebuffer = vulkanCore->GetCurrentSwapchainFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipeline());

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };
		VkBuffer vertexBuffers[] = { vertexBuffer.GetBuffer()};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

		VkDescriptorSet descriptorSetRef = descriptorSet.GetDescriptorSet(vulkanCore->GetCurrentFrame());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 0, 1, &descriptorSetRef, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

private:
	int screenWidth;
	int screenHeight;

	VulkanCore* vulkanCore;
	DescriptorSet descriptorSet;
	ComputeDescriptorSet computeDescriptorSet;
	DescriptorPool descriptorPool;
	UniformBuffer uniformBuffer;
	RaytracerTargetImage raytracerTargetImage;
	Texture texture;
	Sampler sampler;
	GraphicsPipeline graphicsPipeline;
	ComputePipeline computePipeline;
	Buffer vertexBuffer;
	Buffer indexBuffer;
};
