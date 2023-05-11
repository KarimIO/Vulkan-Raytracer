#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "ModelFile.hpp"

void ModelFile::Initialize(const char* path) {
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
		
	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path); // for binary glTF(.glb)
	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}
		
	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		return;
	}

	for (size_t i = 0; i < model.scenes.size(); ++i) {
		const tinygltf::Scene& scene = model.scenes[i];
		std::cout << scene.name << '\n';
		ProcessSceneNodes(scene);
	}

	ProcessMeshes();
}

void ModelFile::ProcessSceneNodes(const tinygltf::Scene& scene) {
	struct Node {
		glm::mat4 modelMatrix;
		int meshIndex;
	};

	for(int nodeIndex : scene.nodes) {
		tinygltf::Node& node = model.nodes[nodeIndex];
		int meshIndex = node.mesh;
		glm::mat4 matrix;
		CopyMatrix(matrix, node.matrix);

		Node finalNode{ matrix, meshIndex };
	}
}

void ModelFile::ProcessMeshes() {
	for (tinygltf::Mesh& mesh : model.meshes) {
		for (tinygltf::Primitive& primitive : mesh.primitives) {
			int material = primitive.material;

			const float* bufferPositions = nullptr;
			const float* bufferNormals = nullptr;

			glm::vec3 posMin;
			glm::vec3 posMax;

			int positionsByteStride = 0;
			int normalsByteStride = 0;

			auto positionsAccessorInMap = primitive.attributes.find("POSITION");
			if (positionsAccessorInMap != primitive.attributes.end()) {
				const tinygltf::Accessor& posAccessor = model.accessors[positionsAccessorInMap->second];
				const tinygltf::BufferView& positionsView = model.bufferViews[posAccessor.bufferView];
				bufferPositions = reinterpret_cast<const float*>(&(model.buffers[positionsView.buffer].data[posAccessor.byteOffset + positionsView.byteOffset]));
				posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
				uint32_t vertexCount = static_cast<uint32_t>(posAccessor.count);
				positionsByteStride = posAccessor.ByteStride(positionsView)
					? (posAccessor.ByteStride(positionsView) / sizeof(float))
					: 3;
			}

			auto normalsAccessorInMap = primitive.attributes.find("NORMAL");
			if (normalsAccessorInMap != primitive.attributes.end()) {
				const tinygltf::Accessor& normalsAccessor = model.accessors[normalsAccessorInMap->second];
				const tinygltf::BufferView& normalsView = model.bufferViews[normalsAccessor.bufferView];
				bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normalsView.buffer].data[normalsAccessor.byteOffset + normalsView.byteOffset]));
				normalsByteStride = normalsAccessor.ByteStride(normalsView)
					? (normalsAccessor.ByteStride(normalsView) / sizeof(float))
					: 3;
			}
		}
	}
}

void ModelFile::CopyMatrix(glm::mat4& dstMatrix, const std::vector<double>& srcMatrix) {
	float* dstMatrixAsFloat = &dstMatrix[0][0];
	for (size_t i = 0; i < srcMatrix.size(); ++i) {
		dstMatrixAsFloat[i] = static_cast<float>(srcMatrix[i]);
	}
}
