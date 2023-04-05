export module RaytracerEngine;

import std.core;

import <GLFW/glfw3.h>;

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

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
		HandleTime();

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
		float mouseSpeed = 1.0f;

		int xpos, ypos;
		vulkanCore.GetMousePos(xpos, ypos);

		int width, height;
		vulkanCore.GetSize(width, height);
		vulkanCore.SetMousePos(width / 2, height / 2);

		horizontalAngle += mouseSpeed * deltaTime * float(width / 2 - xpos);
		verticalAngle += mouseSpeed * deltaTime * float(height / 2 - ypos);

		const float maxAngle = 1.55f;
		if (verticalAngle < -maxAngle) {
			verticalAngle = -maxAngle;
		}
		else if (verticalAngle > maxAngle) {
			verticalAngle = maxAngle;
		}

		glm::quat rotation = glm::quat(glm::vec3(verticalAngle, horizontalAngle, 0.0f));

		cameraForward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 right = rotation * glm::vec3(-1.0f, 0.0f, 0.0f);

		if (vulkanCore.GetKey(GLFW_KEY_UP) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_W) == GLFW_PRESS) {
			cameraPosition += cameraForward * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_DOWN) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_S) == GLFW_PRESS) {
			cameraPosition -= cameraForward * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_RIGHT) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_D) == GLFW_PRESS) {
			cameraPosition += right * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_LEFT) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_A) == GLFW_PRESS) {
			cameraPosition -= right * deltaTime * speed;
		}
	}

	void SetUniformData() {
		glm::mat4 viewMatrix = glm::inverse(glm::lookAt(
			cameraPosition, // the position of your camera, in world space
			cameraPosition + cameraForward,   // where you want to look at, in world space
			glm::vec3(0,1,0)        // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
		));

		renderer.SetUniformData(lastTime, viewMatrix, invProjectionMatrix);
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
	glm::vec3 cameraPosition = glm::vec3(0, 0, -3);
	glm::vec3 cameraForward;
	float horizontalAngle = 0.0f;
	float verticalAngle = 0.0f;

	float lastFrameTime;
	double lastTime;
};
