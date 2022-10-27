#pragma once

#include <unordered_map>
#include <string>
#include "GL/glew.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"


class Manager {
protected:
	static Manager* instance;
	std::unordered_map<std::string, Shader*> shaders;
	std::unordered_map<std::string, Mesh*> meshes;
	std::unordered_map<std::string, Texture*> textures;
	std::unordered_map<std::string, Material*> materials;
	unsigned int counter = 1;

	~Manager() {
		shaders.clear();
		meshes.clear();
		textures.clear();
		materials.clear();
		counter = 1;
		instance = nullptr;
	}

	Manager() {
		shaders["None"] = nullptr;
		meshes["None"] = nullptr;
		materials["None"] = nullptr;
		textures["None"] = nullptr;
	}


public:
	static Manager* getInstance() {
		if (instance == nullptr) 
			instance = new Manager();
		return instance;
	}

	static void destroy() {
		if (instance != nullptr)
			delete instance;
	}

	Shader* addShader(std::string name, Shader* shader) {
		shaders[name] = shader;
		return shader;
	}

	Mesh* addMesh(std::string name, Mesh* mesh) {
		meshes[name] = mesh;
		return mesh;
	}

	Texture* addTexture(std::string name, Texture* texture) {
		textures[name] = texture;
		return texture;
	}

	Material* addMaterial(std::string name, Material* material) {
		materials[name] = material;
		return material;
	}

	Shader* getShader(std::string name) {
		return shaders[name];
	}

	Mesh* getMesh(std::string name) {
		return meshes[name];
	}

	Texture* getTexture(std::string name) {
		return textures[name];
	}

	Material* getMaterial(std::string name) {
		return materials[name];
	}

	std::unordered_map<std::string, Shader*> getShaders() {
		return shaders;
	}

	std::unordered_map<std::string, Mesh*> getMeshes() {
		return meshes;
	}

	std::unordered_map<std::string, Texture*> getTextures() {
		return textures;
	}

	std::unordered_map<std::string, Material*> getMaterials() {
		return materials;
	}

	int getShadersSize() {
		return shaders.size();
	}

	int getMeshesSize() {
		return meshes.size();
	}

	int getTexturesSize() {
		return textures.size();
	}

	int getMaterialsSize() {
		return materials.size();
	}

	void init() {
		for (auto mesh_entry : meshes) {
			if (mesh_entry.second)
			mesh_entry.second->init();
		}
	}

	unsigned int getCounter() {
		return counter; //removed ++ otherwise new obj would increase +2
	}
	void incCounter() {
		counter++; 
	}
	bool hasMesh(string meshname) {
		return meshes.find(meshname) != meshes.end();
	}
};