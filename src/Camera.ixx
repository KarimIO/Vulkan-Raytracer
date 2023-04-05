export module Camera;

import std.core;

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import <GLFW/glfw3.h>;

import VulkanCore;

const float moveSpeed = 1.5f;
const float lookSpeed = 1.5f;

export class Camera {
public:
	void CalculateProjectionMatrix() {
		int width, height;
		VulkanCore::GetVulkanCoreInstance().GetSize(width, height);

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800.0f / (float)600.0f, 0.1f, 100.0f);
		inverseProjectionMatrix = glm::inverse(projection);
	}

	void HandleInput(float deltaTime) {
		VulkanCore& vulkanCore = VulkanCore::GetVulkanCoreInstance();
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

		glm::vec3 cameraForward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 right = rotation * glm::vec3(-1.0f, 0.0f, 0.0f);

		if (vulkanCore.GetKey(GLFW_KEY_UP) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_W) == GLFW_PRESS) {
			position += cameraForward * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_DOWN) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_S) == GLFW_PRESS) {
			position -= cameraForward * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_RIGHT) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_D) == GLFW_PRESS) {
			position += right * deltaTime * speed;
		}

		if (vulkanCore.GetKey(GLFW_KEY_LEFT) == GLFW_PRESS || vulkanCore.GetKey(GLFW_KEY_A) == GLFW_PRESS) {
			position -= right * deltaTime * speed;
		}

		inverseViewMatrix = glm::inverse(glm::lookAt(
			position,
			position + cameraForward,
			glm::vec3(0, 1, 0)
		));

	}

	void ResetCursor() {
		VulkanCore& vulkanCore = VulkanCore::GetVulkanCoreInstance();

		int width, height;
		vulkanCore.GetSize(width, height);
		vulkanCore.SetMousePos(width / 2, height / 2);
	}

	glm::mat4& GetInverseProjectionMatrix() {
		return inverseProjectionMatrix;
	}

	glm::mat4& GetInverseViewMatrix() {
		return inverseViewMatrix;
	}
private:
	glm::mat4 inverseProjectionMatrix;
	glm::mat4 inverseViewMatrix;

	glm::vec3 position = glm::vec3(0, 0, -3);
	
	float horizontalAngle = 0.0f;
	float verticalAngle = 0.0f;
};
