export module Renderer;

import std.core;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <vulkan/vulkan.h>;
import GraphicsPipeline;
import Buffer;
import DescriptorSet;
import DescriptorPool;
import Texture;
import VulkanCore;

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

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

export class Renderer {
public:
	bool Initialize(VulkanCore* vulkanCore) {
		std::cout << "Initializing Renderer...\n";

		this->vulkanCore = vulkanCore;
		texture.Initialize("assets/textures/texture.jpg");
		sampler.Initialize();

		vertexBuffer.Initialize(static_cast<const void*>(vertices.data()), vertices.size() * sizeof(vertices[0]), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		indexBuffer.Initialize(static_cast<const void*>(indices.data()), indices.size() * sizeof(indices[0]), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

		VkVertexInputBindingDescription vertexBindingDescription = Vertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 1> vertexAttributeDescription = Vertex::GetAttributeDescriptions();

		DescriptorPoolBuilder()
			.WithImageSamplers(1)
			.Build(descriptorPool);

		descriptorSet.Initialize(texture, sampler, descriptorPool);
		VkDescriptorSetLayout descriptorSetLayout = descriptorSet.GetLayout();

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

	void Render() {
		VkCommandBuffer commandBuffer = vulkanCore->PrepareFrame();
		if (commandBuffer == nullptr) {
			return;
		}

		RecordCommandBuffer(commandBuffer);
		vulkanCore->SubmitFrame();
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

		VkDescriptorSet descriptorSetRef = descriptorSet.GetDescriptorSet();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 0, 1, &descriptorSetRef, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

private:
	VulkanCore* vulkanCore;
	DescriptorSet descriptorSet;
	DescriptorPool descriptorPool;
	Texture texture;
	Sampler sampler;
	GraphicsPipeline graphicsPipeline;
	Buffer vertexBuffer;
	Buffer indexBuffer;
};
