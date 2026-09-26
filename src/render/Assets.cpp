#include "Assets.h"

#include <iostream>

#include "BmpLoader.h"
#include "ProceduralTextures.h"
#include "core/Paths.h"
#include "geometry/Primitives.h"

void Assets::Load()
{
	plane = std::make_unique<Mesh>("Plane", PrimitiveType::Plane, Primitives::Plane());
	cube = std::make_unique<Mesh>("Cube", PrimitiveType::Cube, Primitives::Cube());
	sphere = std::make_unique<Mesh>("Sphere", PrimitiveType::Sphere, Primitives::Sphere());
	cylinder = std::make_unique<Mesh>("Cylinder", PrimitiveType::Cylinder, Primitives::Cylinder());
	cone = std::make_unique<Mesh>("Cone", PrimitiveType::Cone, Primitives::Cone());

	namespace PT = ProceduralTextures;
	white = AddTexture("white", PT::White());
	AddTexture("wood-floor", PT::WoodFloor(), FloorSlot);
	AddTexture("wallpaper", PT::Wallpaper(), WallSlot);
	AddTexture("rug", PT::Rug(), RugSlot);
	AddTexture("beach-ball", PT::BeachBall(), BallSlot);
	AddTexture("toy-block", PT::ToyBlock(128, glm::vec3(1.0f)), BlockSlot);
	AddTexture("night-sky", PT::NightSky(), StarsSlot);

	// The poster is a real image file read by our own BMP loader.
	const std::string posterPath = Paths::resolve("assets/textures/poster.bmp").string();
	if (auto poster = Bmp::Load(posterPath)) {
		AddTexture("poster", *poster, PosterSlot);
		std::cout << "Loaded " << posterPath << " (" << poster->width << "x" << poster->height << ")\n";
	}
	else {
		AddTexture("poster", PT::PosterFallback(), PosterSlot);
	}
}

const Texture* Assets::AddTexture(const std::string& name, const Image& image, int slot, bool nearest)
{
	textures.push_back(std::make_unique<Texture>(name, image, nearest));
	const Texture* tex = textures.back().get();
	if (slot >= 0)
		slotTextures[static_cast<size_t>(slot)] = tex;
	return tex;
}

Material& Assets::Mat(const std::string& name, const glm::vec3& color, float ks, float shininess)
{
	auto it = materials.find(name);
	if (it != materials.end())
		return *it->second;
	auto m = std::make_unique<Material>();
	m->name = name;
	m->color = color;
	m->ks = ks;
	m->shininess = shininess;
	Material& ref = *m;
	materials.emplace(name, std::move(m));
	return ref;
}

Material* Assets::Find(const std::string& name)
{
	auto it = materials.find(name);
	return it == materials.end() ? nullptr : it->second.get();
}
