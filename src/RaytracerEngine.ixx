export module RaytracerEngine;

import <iostream>;
import VulkanCore;

export class RaytracerEngine {
public:
	bool Initialize() {
		std::cout << "Initializing Raytracer Engine...\n";
		if (!vulkanCore.Initialize()) {
			return false;
		}

		std::cout << "Initialized Raytracer Engine.\n";
		return true;
	}

	void Run() {
		while (!vulkanCore.ShouldClose()) {
			vulkanCore.PollEvents();
			vulkanCore.DrawFrame();
		}
	}

private:
	VulkanCore vulkanCore;
};
