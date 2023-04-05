export module RaytracerEngine;

import std.core;

import <GLFW/glfw3.h>;

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

import Camera;
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
		HandleTime();

		camera.CalculateProjectionMatrix();
		camera.ResetCursor();

		std::cout << "Initialized Raytracer Engine.\n";
		return true;
	}

	~RaytracerEngine() {
		vulkanCore.HideWindow();
	}

	void Run() {
		while (!vulkanCore.ShouldClose()) {
			HandleTime();
			camera.HandleInput(deltaTime);
			SetUniformData();
			vulkanCore.PollEvents();
			renderer.Render();
		}

		vulkanCore.WaitUntilEndOfFrame();
	}

	void SetUniformData() {
		renderer.SetUniformData(lastTime, camera.GetInverseViewMatrix(), camera.GetInverseProjectionMatrix());
	}

	void HandleTime() {
		double currentTime = glfwGetTime();
		deltaTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;
	}

private:
	VulkanCore vulkanCore;
	Renderer renderer;
	Camera camera;

	float deltaTime;
	double lastTime;
};
