#if false
#pragma once

#include <iostream>
#include <vector>
#include <tiny_gltf.h>

class ModelFile {
#else
export module ModelFile;

import std.core;
import <tiny_gltf.h>;

export class ModelFile {
#endif
public:
	void Initialize(const char* path) {
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

		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < model.scenes.size(); ++i) {
			std::cout << scene.name << '\n';
		}
	}

	void ProcessScene(tinygltf::Scene& scene) {
		for(auto& node : scene.nodes) {
		}
	}
private:
	tinygltf::Model model;
};