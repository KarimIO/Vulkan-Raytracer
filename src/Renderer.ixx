export module Renderer;

import std.core;

import "ModelFile.hpp";

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

import <vulkan/vulkan.h>;

import Buffer;
import ComputePipeline;
import ComputeDescriptorSet;
import DebugWindow;
import DescriptorSet;
import DescriptorPool;
import GraphicsPipeline;
import Texture;
// import ModelFile;
import RaytracerTargetImage;
import VulkanCore;

struct ScreenSpaceVertex {
	glm::vec2 pos;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(ScreenSpaceVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> GetAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(ScreenSpaceVertex, pos);

		return attributeDescriptions;
	}
};

const std::vector<ScreenSpaceVertex> vertices = {
	{{0.0f, 0.0f}},
	{{1.0f, 0.0f}},
	{{1.0f, 1.0f}},
	{{0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 2, 1, 3, 2, 0
};

struct RenderUniformBufferObject {
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
	float dofStrength;
	float blurStrength;
	uint32_t framesSinceLastMove = 0;
};

struct Material {
	alignas(16) glm::vec3 albedo = glm::vec3(0.0f, 0.0f, 0.0f);
	alignas(16) glm::vec3 specular = glm::vec3(0.04f, 0.04f, 0.04f);
	float specularChance = 0.04f;
	float ior = 1.0f;
	alignas(16) glm::vec3 emission = glm::vec3(0.0f, 0.0f, 0.0f);
	float surfaceRoughness = 0.5f;
	float refractionRoughness = 0.0f;
	alignas(16) glm::vec3 transmissionColor = glm::vec3(0.0f, 0.0f, 0.0f);
	float refractionChance = 0.0f;

	static Material Emissive(glm::vec3 emission) {
		Material material;
		material.emission = emission;

		return material;
	}

	static Material Diffuse(glm::vec3 albedoColor, float roughness, float ior) {
		Material material;
		material.albedo = albedoColor;
		material.surfaceRoughness = roughness;
		material.ior = ior;
		material.specularChance = 0.04f;

		return material;
	}

	static Material Metal(glm::vec3 specularColor, float roughness, float ior) {
		Material material;
		material.specular = specularColor;
		material.surfaceRoughness = roughness;
		material.ior = ior;
		material.specularChance = 1.0f;

		return material;
	}

	static Material Transmissive(glm::vec3 transmissionColor, float refractionRoughness, float ior) {
		Material material;
		material.albedo = glm::vec3(1.0f, 1.0f, 1.0f);
		material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
		material.transmissionColor = transmissionColor;
		material.surfaceRoughness = 0.0f;
		material.refractionRoughness = refractionRoughness;
		material.ior = ior;
		material.refractionChance = 1.0f;
		material.specularChance = 0.0f;

		return material;
	}
};

struct MaterialUniformBufferObject {
	Material materials[32];
};

struct Sphere {
	alignas(16) glm::vec3 center;
	float radius;
	uint32_t materialIndex;
};

struct SphereUniformBufferObject {
	uint32_t numSpheres;
	Sphere spheres[64];
};

struct Vertex {
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 normal;
};

struct VertexUniformBufferObject {
	Vertex vertices[128];
};

struct IndexUniformBufferObject {
	struct alignas(16) FourByteAlignedUint {
		uint32_t value;
	};

	FourByteAlignedUint indices[128];
};

struct MeshInfo {
	uint32_t baseIndex;
	uint32_t indexCount;
	uint32_t materialIndex;
	alignas(16) glm::vec3 boundsMin;
	alignas(16) glm::vec3 boundsMax;
};

struct MeshInfoUniformBufferObject {
	uint32_t numMeshes;
	MeshInfo meshes[64];
};

export class Renderer {
public:
	bool Initialize(VulkanCore* vulkanCore, DebugWindow* debugWindow, Settings* settings) {
		std::cout << "Initializing Renderer...\n";

		this->settings = settings;
		this->vulkanCore = vulkanCore;
		this->debugWindow = debugWindow;
		vulkanCore->PassResizeFramebufferCallback([&](int w, int h) { this->ResizeFramebufferCallback(w, h); });

		VulkanCore::GetVulkanCoreInstance().GetSize(screenWidth, screenHeight);

		raytracerTargetImage.Initialize(screenWidth, screenHeight);
		sampler.Initialize();

		vertexBuffer.Initialize(static_cast<const void*>(vertices.data()), vertices.size() * sizeof(vertices[0]), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		indexBuffer.Initialize(static_cast<const void*>(indices.data()), indices.size() * sizeof(indices[0]), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

		renderUniformBuffer.Initialize(2, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(RenderUniformBufferObject));
		materialUniformBuffer.Initialize(1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(MaterialUniformBufferObject));
		sphereUniformBuffer.Initialize(1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(SphereUniformBufferObject));
		vertexUniformBufferObject.Initialize(1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(VertexUniformBufferObject));
		indexUniformBufferObject.Initialize(1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(IndexUniformBufferObject));
		meshInfoUniformBufferObject.Initialize(1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(MeshInfoUniformBufferObject));

		SetupMaterials();
		SetupSpheres();
		SetupMeshes();

		VkVertexInputBindingDescription vertexBindingDescription = ScreenSpaceVertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 1> vertexAttributeDescription = ScreenSpaceVertex::GetAttributeDescriptions();

		DescriptorPoolBuilder()
			.WithStorageImages(MAX_FRAMES_IN_FLIGHT)
			.WithUniformBuffers(MAX_FRAMES_IN_FLIGHT)
			.WithImageSamplers(MAX_FRAMES_IN_FLIGHT)
			.Build(descriptorPool);

		descriptorSet.Initialize(raytracerTargetImage, sampler, descriptorPool);
		computeDescriptorSet.Initialize(raytracerTargetImage, sampler, renderUniformBuffer, materialUniformBuffer, sphereUniformBuffer, vertexUniformBufferObject, indexUniformBufferObject, meshInfoUniformBufferObject, descriptorPool);
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
		std::cout << "Resized to " << width << ", " << height << '\n';
		ResetFrameCounter();
		screenWidth = width;
		screenHeight = height;
		raytracerTargetImage.Resize(width, height);
		descriptorSet.UpdateTargetImage(raytracerTargetImage, sampler);
		computeDescriptorSet.UpdateTargetImage(raytracerTargetImage, sampler);
	}

	void SetupMeshes() {
		VertexUniformBufferObject& vertexUbo = vertexUniformBufferObject.GetMappedBuffer<VertexUniformBufferObject>();
		IndexUniformBufferObject& indexUbo = indexUniformBufferObject.GetMappedBuffer<IndexUniformBufferObject>();

		std::vector<Vertex> vertices{
			//Top
			Vertex(glm::vec3(4, 1, -1), glm::vec3(0, 1, 0)), //0
			Vertex(glm::vec3(6, 1, -1), glm::vec3(0, 1, 0)),  //1
			Vertex(glm::vec3(4, 1, 1), glm::vec3(0, 1, 0)), //2
			Vertex(glm::vec3(6, 1,  1), glm::vec3(0, 1, 0)), //3
				
			//Bottom
			Vertex(glm::vec3(4, -1, -1), glm::vec3(0, -1, 0)), //4
			Vertex(glm::vec3(6, -1, -1), glm::vec3(0, -1, 0)), //5
			Vertex(glm::vec3(4, -1,  1), glm::vec3(0, -1, 0)), //6
			Vertex(glm::vec3(6, -1,  1), glm::vec3(0, -1, 0)),  //7
				
			//Front
			Vertex(glm::vec3(4, 1, 1), glm::vec3(0, 0, 1)), //8
			Vertex(glm::vec3(6,  1, 1), glm::vec3(0, 0, 1)), //9
			Vertex(glm::vec3(4, -1, 1), glm::vec3(0, 0, 1)), //10
			Vertex(glm::vec3(6, -1, 1), glm::vec3(0, 0, 1)), //11
				
			//Back
			Vertex(glm::vec3(4,  1, -1), glm::vec3(0, 0, -1)), //12
			Vertex(glm::vec3(6,  1, -1), glm::vec3(0, 0, -1)), //13
			Vertex(glm::vec3(4, -1, -1), glm::vec3(0, 0, -1)), //14
			Vertex(glm::vec3(6, -1, -1), glm::vec3(0, 0, -1)), //15
				
			//Left
			Vertex(glm::vec3(4,  1,  1), glm::vec3(-1, 0, 0)), //16
			Vertex(glm::vec3(4,  1, -1), glm::vec3(-1, 0, 0)), //17
			Vertex(glm::vec3(4, -1,  1), glm::vec3(-1, 0, 0)), //18
			Vertex(glm::vec3(4, -1, -1), glm::vec3(-1, 0, 0)), //19
				
			//Right
			Vertex(glm::vec3(6,  1,  1), glm::vec3(1, 0, 0)), //20
			Vertex(glm::vec3(6,  1, -1), glm::vec3(1, 0, 0)), //21
			Vertex(glm::vec3(6, -1,  1), glm::vec3(1, 0, 0)), //22
			Vertex(glm::vec3(6, -1, -1), glm::vec3(1, 0, 0))  //23
		};

		std::vector<uint32_t> indices{
			//Top
			1, 0, 2,
			2, 3, 1,

			//Bottom
			4, 5, 6,
			7, 6, 5,

			//Front
			9, 8, 10,
			10, 11, 9,

			//Back
			12, 13, 14,
			15, 14, 13,

			//Left
			16, 17, 18,
			19, 18, 17,

			//Right
			21, 20, 22,
			22, 23, 21
		};

		for (size_t i = 0; i < vertices.size(); ++i) {
			vertexUbo.vertices[i] = vertices[i];
		}

		for (size_t i = 0; i < indices.size(); ++i) {
			indexUbo.indices[i].value = indices[i];
		}

		modelFile.Initialize("assets/model/box01.glb");

		MeshInfoUniformBufferObject& meshUbo = meshInfoUniformBufferObject.GetMappedBuffer<MeshInfoUniformBufferObject>();
		meshUbo.numMeshes = 1;
		meshUbo.meshes[0].baseIndex = 0;
		meshUbo.meshes[0].indexCount = static_cast<uint32_t>(indices.size());
		meshUbo.meshes[0].materialIndex = 6;
		meshUbo.meshes[0].boundsMin = glm::vec3(4.0f, -1.0f, -1.0f);
		meshUbo.meshes[0].boundsMax = glm::vec3(6.0f,  1.0f,  1.0f);
	}

	void SetupSpheres() {
		SphereUniformBufferObject& ubo = sphereUniformBuffer.GetMappedBuffer<SphereUniformBufferObject>();

		uint32_t currentSphere = 1;
		ubo.spheres[0] = Sphere{ glm::vec3(0, 11, 0), 10, 0 };

		for (uint32_t i = 0; i < 3; ++i) {
			for (int j = -2; j <= 2; ++j) {
				ubo.spheres[++currentSphere] = Sphere{ glm::vec3(j, 0.2f - i * 1.2f, 0.0f), 0.4f, currentSphere };
			}
		}

		ubo.numSpheres = currentSphere + 1;
	}

	void SetupMaterials() {
		MaterialUniformBufferObject& ubo = materialUniformBuffer.GetMappedBuffer<MaterialUniformBufferObject>();
		ubo.materials[0] = Material::Emissive(glm::vec3(4.0f));

		size_t materialIndex = 1;

		for (int i = 0; i < 5; ++i) {
			ubo.materials[materialIndex++] = Material::Diffuse(glm::vec3(1.0f, 0.0f, 0.0f), 0.0f + i * 0.2f, 1.5f);
		}

		for (int i = 0; i < 5; ++i) {
			ubo.materials[materialIndex++] = Material::Metal(glm::vec3(0.944, 0.776f, 0.373f), 0.0f + i * 0.2f, 2.950f);
		}

		for (int i = 0; i < 5; ++i) {
			ubo.materials[materialIndex++] = Material::Transmissive(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f + i * 0.2f, 1.52f);
		}
	}

	void SetUniformData(double time, glm::mat4& cameraToWorld, glm::mat4& cameraInverseProj) {
		uint32_t currentFrame = vulkanCore->GetCurrentFrame();

		RenderUniformBufferObject& ubo = renderUniformBuffer.GetMappedBuffer<RenderUniformBufferObject>(currentFrame);
		ubo.cameraToWorld = cameraToWorld;
		ubo.cameraInverseProj = cameraInverseProj;
		ubo.time = static_cast<float>(time);
		ubo.maxRayBounceCount = settings->numBounces;
		ubo.numRaysPerPixel = framesSinceLastMove < 2
			? settings->numRaysWhileMoving
			: settings->numRaysWhileStill;

		glm::quat rotation(glm::vec3(settings->sunLightPitch, settings->sunLightYaw, 0.0f));
		glm::vec3 cameraForward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
		ubo.sunLightDirection = cameraForward;
		ubo.skyColor = settings->skyColor;
		ubo.horizonColor = settings->horizonColor;
		ubo.groundColor = settings->groundColor;
		ubo.sunFocus = settings->sunFocus;
		ubo.sunIntensity = settings->sunIntensity;
		ubo.dofStrength = 1.0f;
		ubo.blurStrength = 1.0f;
		ubo.framesSinceLastMove = framesSinceLastMove++;
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

			RecordBlitCommandBuffer(graphicsCommandBuffer);
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

		uint32_t groupCountX = static_cast<uint32_t>(std::ceil(screenWidth / 16.0f));
		uint32_t groupCountY = static_cast<uint32_t>(std::ceil(screenHeight / 16.0f));

		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}

	void RecordBlitCommandBuffer(VkCommandBuffer commandBuffer) {
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

		debugWindow->Render(commandBuffer);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void ResetFrameCounter() {
		framesSinceLastMove = 0;
	}

private:
	int screenWidth;
	int screenHeight;

	VulkanCore* vulkanCore;
	Settings* settings;
	DebugWindow* debugWindow;
	DescriptorSet descriptorSet;
	ComputeDescriptorSet computeDescriptorSet;
	DescriptorPool descriptorPool;
	UniformBuffer renderUniformBuffer;
	UniformBuffer materialUniformBuffer;
	UniformBuffer sphereUniformBuffer;
	UniformBuffer vertexUniformBufferObject;
	UniformBuffer indexUniformBufferObject;
	UniformBuffer meshInfoUniformBufferObject;
	RaytracerTargetImage raytracerTargetImage;
	Texture texture;
	Sampler sampler;
	GraphicsPipeline graphicsPipeline;
	ComputePipeline computePipeline;
	Buffer vertexBuffer;
	Buffer indexBuffer;
	ModelFile modelFile;

	uint32_t framesSinceLastMove = 0;
};
