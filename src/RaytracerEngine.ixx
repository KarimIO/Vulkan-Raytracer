export module RaytracerEngine;

import std.core;

import <GLFW/glfw3.h>;

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
			HandleInput();
			SetUniformData();
			vulkanCore.PollEvents();
			renderer.Render();
		}

		vulkanCore.WaitUntilEndOfFrame();
	}

	void HandleInput() {
		float deltaTime = lastFrameTime;
		float speed = 1.0f;
		float mouseSpeed = 10.0f;

		int xpos, ypos;
		vulkanCore.GetMousePos(xpos, ypos);

		int width, height;
		vulkanCore.GetSize(width, height);
		vulkanCore.SetMousePos(width / 2, height / 2);

		horizontalAngle += mouseSpeed * deltaTime * float(1024 / 2 - xpos);
		verticalAngle += mouseSpeed * deltaTime * float(768 / 2 - ypos);

		glm::vec3 forward(0, 0, -1);

		glm::vec3 right = glm::vec3(1, 0, 0);

		if (vulkanCore.GetKey(GLFW_KEY_UP) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_W) == GLFW_PRESS) {
			cameraPosition += forward * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_DOWN) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_S) == GLFW_PRESS) {
			cameraPosition -= forward * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_RIGHT) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_D) == GLFW_PRESS) {
			cameraPosition += right * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_LEFT) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_A) == GLFW_PRESS) {
			cameraPosition -= right * deltaTime * speed;
		}
	}

	void SetUniformData() {
		glm::mat4 camToWorldMatrix = glm::translate(glm::mat4(1), cameraPosition);
		camToWorldMatrix = glm::rotate(camToWorldMatrix, 0.2f, glm::vec3(0, 1, 0));
		renderer.SetUniformData(lastTime, camToWorldMatrix, invProjectionMatrix);
	}

	void HandleTime() {
		double currentTime = glfwGetTime();
		lastFrameTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;
	}

private:
	VulkanCore vulkanCore;
	Renderer renderer;

	glm::mat4 invProjectionMatrix;
	glm::vec3 cameraPosition = glm::vec3(0, 0, 0);
	float horizontalAngle = 0.0f;
	float verticalAngle = 0.0f;

	float lastFrameTime;
	double lastTime;
};
