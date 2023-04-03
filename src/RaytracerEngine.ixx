export module RaytracerEngine;

import std.core;
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
