#pragma once

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "geometry/Mesh.h"
#include "gl/Texture.h"
#include "scene/Material.h"

// Owns every shared GPU resource: the five primitive meshes, all textures and all materials.
// Objects in the scene only hold pointers into this library, so e.g. every sphere in the scene
// (heads, eyes, hands, the ball...) is drawn from the SAME vertex buffer.
class Assets {
public:
	// Layers in the ray tracer's shared surface texture array.
	enum TextureSlot : int { FloorSlot = 0, WallSlot, RugSlot, BallSlot, BlockSlot, PosterSlot, StarsSlot,
		MoonSlot, FabricSlot, DenimSlot, LeatherSlot, PlaidSlot, CowSlot, BookSlot, PicketSlot,
		SidingSlot, ShingleSlot, BrickSlot, GrassSlot, WindowSlot, FlowerSlot, SlotCount };

	void Load();
	~Assets();
	GLuint SurfaceArray() const { return surfaceArray; }
	int NamedSlot(const std::string& name) const;
	void ExportTextures(const std::string& directory) const;

	const Mesh& Plane() const { return *plane; }
	const Mesh& Cube() const { return *cube; }
	const Mesh& Sphere() const { return *sphere; }
	const Mesh& Cylinder() const { return *cylinder; }
	const Mesh& Cone() const { return *cone; }
	std::vector<const Mesh*> AllMeshes() const { return { plane.get(), cube.get(), sphere.get(), cylinder.get(), cone.get() }; }

	const Texture& WhiteTexture() const { return *white; }
	// Named surface textures, including the house exterior; nullptr if unknown.
	const Texture* Named(const std::string& name) const;
	const Texture* SlotTexture(int slot) const { return slotTextures[static_cast<size_t>(slot)]; }

	// Creates (or returns the existing) material with this name.
	Material& Mat(const std::string& name, const glm::vec3& color, float ks = 0.25f, float shininess = 24.0f);
	Material* Find(const std::string& name);

private:
	const Texture* AddTexture(const std::string& name, const Image& image, int slot = -1, bool nearest = false);

	std::unique_ptr<Mesh> plane, cube, sphere, cylinder, cone;
	std::vector<std::unique_ptr<Mesh>> detailLevels; // coarser sphere / cylinder / cone versions
	std::vector<std::unique_ptr<Texture>> textures;
	std::array<const Texture*, SlotCount> slotTextures{};
	const Texture* white = nullptr;
	GLuint surfaceArray = 0;
	std::map<std::string, Image> sourceImages;
	std::map<std::string, int> namedSlots;
	std::map<std::string, const Texture*> named;
	std::map<std::string, std::unique_ptr<Material>> materials;
};
