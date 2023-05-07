export module Settings;

import std.core;
import <glm/glm.hpp>;
import <glm/gtc/constants.hpp>;

export class Settings {
public:
	int numBounces = 3;
	int numRaysWhileStill = 50;
	int numRaysWhileMoving = 5;
	float sunLightPitch = -32.0f * glm::pi<float>() / 180.0f;
	float sunLightYaw = 10.0f * glm::pi<float>() / 180.0f;
	glm::vec3 skyColor = glm::vec3(0.11, 0.36, 0.57);
	glm::vec3 horizonColor = glm::vec3(0.83, 0.82, 0.67);
	glm::vec3 groundColor = glm::vec3(0.2, 0.2, 0.2);
	float sunFocus = 200.0f;
	float sunIntensity = 30.0f;
};
