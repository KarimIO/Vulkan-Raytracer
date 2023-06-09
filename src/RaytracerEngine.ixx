export module RaytracerEngine;

import std.core;

import <GLFW/glfw3.h>;

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

import Camera;
import DebugWindow;
import Renderer;
import VulkanCore;

export class RaytracerEngine {
public:
	bool Initialize() {
		std::cout << "Initializing Raytracer Engine...\n";

		if (!vulkanCore.Initialize()) {
			return false;
		}

		if (!debugWindow.Initialize(&settings, std::bind(&Renderer::ResetFrameCounter, &renderer))) {
			return false;
		}

		if (!renderer.Initialize(&vulkanCore, &debugWindow, &settings)) {
			return false;
		}

		vulkanCore.PassResizeFramebufferCallback([&](int w, int h) { this->ResizeFramebufferCallback(w, h); });
		vulkanCore.ShowWindow();
		vulkanCore.SetCursorVisible(true);
		HandleTime();

		camera.CalculateProjectionMatrix();
		camera.ResetCursor();

		std::cout << "Initialized Raytracer Engine.\n";
		return true;
	}

	~RaytracerEngine() {
		vulkanCore.HideWindow();
	}

	void ResizeFramebufferCallback(int width, int height) {
		camera.CalculateProjectionMatrix(width, height);
	}

	void Run() {
		while (!vulkanCore.ShouldClose()) {
			vulkanCore.PollEvents();
			HandleTime();
			HandleInput();
			SetUniformData();

			debugWindow.Update();

			renderer.Render();
		}

		vulkanCore.WaitUntilEndOfFrame();
	}

	void HandleInput() {
		if (vulkanCore.GetKey(GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			vulkanCore.Close();
		}
		
		bool isTabPressed = vulkanCore.GetKey(GLFW_KEY_TAB) == GLFW_PRESS;
		if (isTabPressed && !wasTabPressed) {
			camera.ToggleInputEnabled();
		}

		wasTabPressed = isTabPressed;

		bool hasMoved = camera.HandleInput(deltaTime);
		if (hasMoved) {
			renderer.ResetFrameCounter();
		}
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
	DebugWindow debugWindow;
	Renderer renderer;
	Camera camera;
	Settings settings;

	float deltaTime;
	double lastTime;
	bool wasTabPressed;
};
