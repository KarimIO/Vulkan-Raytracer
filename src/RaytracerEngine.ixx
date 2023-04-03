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
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800.0f / (float)600.0f, 0.1f, 100.0f);
		projection[1][1] *= -1;

		glm::mat4 view = glm::lookAt(
			glm::vec3(4, 3, 3), // Camera is at (4,3,3), in World Space
			glm::vec3(0, 0, 0), // and looks at the origin
			glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
		);

		glm::mat4 finalMatrix = projection * view;
		renderer.SetUniformData(lastTime, finalMatrix);

		while (!vulkanCore.ShouldClose()) {
			HandleTime();
			vulkanCore.PollEvents();
			renderer.Render();
		}

		vulkanCore.WaitUntilEndOfFrame();
	}

	void HandleTime() {
		double currentTime = glfwGetTime();
		lastFrameTime = static_cast<float>(currentTime - lastTime) * 1000.0f;
		lastTime = currentTime;
	}

private:
	VulkanCore vulkanCore;
	Renderer renderer;

	float lastFrameTime;
	double lastTime;
};
