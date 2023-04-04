export module RaytracerEngine;

import std.core;

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

import Renderer;
import VulkanCore;

export class RaytracerEngine {
public:
	bool Initialize() {
		std::cout << "Initializing Raytracer Engine...\n";

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800.0f / (float)600.0f, 0.1f, 100.0f);
		invProjectionMatrix = glm::inverse(projection);

		if (!vulkanCore.Initialize()) {
			return false;
		}

		if (!renderer.Initialize(&vulkanCore)) {
			return false;
		}

		vulkanCore.ShowWindow();

		std::cout << "Initialized Raytracer Engine.\n";
		return true;
	}

	~RaytracerEngine() {
		vulkanCore.HideWindow();
	}

	void Run() {
		while (!vulkanCore.ShouldClose()) {
			HandleTime();
			SetUniformData();
			vulkanCore.PollEvents();
			renderer.Render();
		}

		vulkanCore.WaitUntilEndOfFrame();
	}

	void SetUniformData() {
		glm::mat4 camToWorldMatrix = glm::translate(glm::mat4(1), glm::vec3(0.5, 0, 0));
		camToWorldMatrix = glm::rotate(camToWorldMatrix, 0.2f, glm::vec3(0, 1, 0));
		renderer.SetUniformData(lastTime, camToWorldMatrix, invProjectionMatrix);
	}

	void HandleTime() {
		double currentTime = glfwGetTime();
		lastFrameTime = static_cast<float>(currentTime - lastTime) * 1000.0f;
		lastTime = currentTime;
	}

private:
	VulkanCore vulkanCore;
	Renderer renderer;
	glm::mat4 invProjectionMatrix;

	float lastFrameTime;
	double lastTime;
};
