export module Material;

import std.core;
import <glm/glm.hpp>;

export struct Material {
	alignas(16) glm::vec3 albedo = glm::vec3(0.0f, 0.0f, 0.0f);
	alignas(16) glm::vec3 specular = glm::vec3(0.04f, 0.04f, 0.04f);
	float specularChance = 0.04f;
	float ior = 1.0f;
	alignas(16) glm::vec3 emission = glm::vec3(0.0f, 0.0f, 0.0f);
	float surfaceRoughness = 0.5f;
	float refractionRoughness = 0.0f;
	alignas(16) glm::vec3 transmissionColor = glm::vec3(0.0f, 0.0f, 0.0f);
	float refractionChance = 0.0f;

	static Material Emissive(glm::vec3 emission) {
		Material material;
		material.emission = emission;

		return material;
	}

	static Material Diffuse(glm::vec3 albedoColor, float roughness, float ior) {
		Material material;
		material.albedo = albedoColor;
		material.surfaceRoughness = roughness;
		material.ior = ior;
		material.specularChance = 0.04f;

		return material;
	}

	static Material Metal(glm::vec3 specularColor, float roughness, float ior) {
		Material material;
		material.specular = specularColor;
		material.surfaceRoughness = roughness;
		material.ior = ior;
		material.specularChance = 1.0f;

		return material;
	}

	static Material Transmissive(glm::vec3 transmissionColor, float refractionRoughness, float ior) {
		Material material;
		material.albedo = glm::vec3(1.0f, 1.0f, 1.0f);
		material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
		material.transmissionColor = transmissionColor;
		material.surfaceRoughness = 0.0f;
		material.refractionRoughness = refractionRoughness;
		material.ior = ior;
		material.refractionChance = 1.0f;
		material.specularChance = 0.0f;

		return material;
	}
};
