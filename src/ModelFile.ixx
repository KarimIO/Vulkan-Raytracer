export module ModelFile;

import std.core;

import <glm/glm.hpp>;
import <glm/gtx/transform.hpp>;

import <tiny_gltf.h>;

import Material;
import Scene;

export class ModelFile {
public:
	void Initialize(const char* path, Scene& dstScene) {
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;
			
		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
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
			ProcessSceneNodes(scene, dstScene);
		}

		ProcessMeshes(dstScene);
		ProcessMaterials(dstScene);
	}

	void ProcessSceneNodes(const tinygltf::Scene& scene, Scene& dstScene) {
		dstScene.nodes.reserve(scene.nodes.size());

		for(int nodeIndex : scene.nodes) {
			tinygltf::Node& node = model.nodes[nodeIndex];
			int meshIndex = node.mesh;
			glm::mat4 matrix;
			CopyMatrix(matrix, node.matrix);

			dstScene.nodes.emplace_back(matrix, meshIndex);
		}
	}

	void ProcessMaterials(Scene& dstScene) {
		dstScene.materials.resize(model.materials.size());

		size_t index = 0;
		for (tinygltf::Material& material : model.materials) {
			Material& dstMaterial = dstScene.materials[index++];
			
			dstMaterial.emission = material.emissiveFactor.size() >= 3
				? glm::vec3(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2])
				: glm::vec3(0.0f);

			auto& pbr = material.pbrMetallicRoughness;

			dstMaterial.albedo = Lerp(
				glm::vec3(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]),
				glm::vec3(0.0f),
				pbr.metallicFactor
			);

			dstMaterial.specular = Lerp(
				glm::vec3(0.04f),
				glm::vec3(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]),
				pbr.metallicFactor
			);

			dstMaterial.surfaceRoughness = dstMaterial.refractionRoughness = pbr.roughnessFactor;
		}
	}

	void ProcessMeshes(Scene& dstScene) {
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

	void CopyMatrix(glm::mat4& dstMatrix, const std::vector<double>& srcMatrix) {
		float* dstMatrixAsFloat = &dstMatrix[0][0];
		for (size_t i = 0; i < srcMatrix.size(); ++i) {
			dstMatrixAsFloat[i] = static_cast<float>(srcMatrix[i]);
		}
	}

	glm::vec3 Lerp(glm::vec3 a, glm::vec3 b, float t) {
		return glm::vec3(
			a.x * t + b.x * (1.0f - t),
			a.y * t + b.y * (1.0f - t),
			a.z * t + b.z * (1.0f - t)
		);
	}

private:
	tinygltf::Model model;
};
