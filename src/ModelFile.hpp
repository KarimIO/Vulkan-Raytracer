#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <tiny_gltf.h>

class ModelFile {
public:
	void Initialize(const char* path);
private:
	void ProcessSceneNodes(const tinygltf::Scene& scene);
	void ProcessMeshes();
	void CopyMatrix(glm::mat4& dstMatrix, const std::vector<double>& srcMatrix);
private:
	tinygltf::Model model;

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
	};

	std::vector<Vertex> vertices;
};
