export module Scene;

import std.core;
import <glm/glm.hpp>;
import Material;

export struct Scene {
	struct Submesh {
		uint32_t baseIndex;
		uint32_t indexCount;
		uint32_t materialIndex;
	};

	struct Mesh {
		int submeshOffsetIndex;
		int submeshCount;
		glm::vec3 oobbMin;
		glm::vec3 oobbMax;
	};

	struct MeshForShader {
		int submeshOffsetIndex;
		int submeshCount;
	};

	struct Node {
		glm::mat4 matrix;
		int meshIndex;

		// Axis-Aligned Bounding Box
		alignas(16) glm::vec3 aabbMin;
		alignas(16) glm::vec3 aabbMax;
	};

	std::vector<int> textures;
	std::vector<Material> materials;
	std::vector<int> indices;
	std::vector<float> vertexData;
	std::vector<Submesh> submeshes;
	std::vector<Mesh> meshes;
	std::vector<Node> nodes;
};
