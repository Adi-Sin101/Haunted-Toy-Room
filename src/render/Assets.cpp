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
	// Full detail is used only for objects that are large on screen; smooth shading and textures
	// carry the surface detail, so 24 x 36 is already round at full-window size (1,656 triangles,
	// was 32 x 48 = 2,976). Small and distant copies use the coarser levels below.
	sphere = std::make_unique<Mesh>("Sphere", PrimitiveType::Sphere, Primitives::Sphere(24, 36));
	cylinder = std::make_unique<Mesh>("Cylinder", PrimitiveType::Cylinder, Primitives::Cylinder(32));
	cone = std::make_unique<Mesh>("Cone", PrimitiveType::Cone, Primitives::Cone(32));
	auto level = [&](const char* name, PrimitiveType type, MeshData data) {
		detailLevels.push_back(std::make_unique<Mesh>(name, type, std::move(data)));
		return detailLevels.back().get();
	};
	sphere->SetDetailLevels(level("Sphere (medium)", PrimitiveType::Sphere, Primitives::Sphere(12, 18)),
		level("Sphere (low)", PrimitiveType::Sphere, Primitives::Sphere(6, 10)));
	cylinder->SetDetailLevels(level("Cylinder (medium)", PrimitiveType::Cylinder, Primitives::Cylinder(16)),
		level("Cylinder (low)", PrimitiveType::Cylinder, Primitives::Cylinder(8)));
	cone->SetDetailLevels(level("Cone (medium)", PrimitiveType::Cone, Primitives::Cone(16)),
		level("Cone (low)", PrimitiveType::Cone, Primitives::Cone(8)));

	namespace PT = ProceduralTextures;
	white = AddTexture("white", PT::White());
	AddTexture("wood-floor", PT::WoodFloor(), FloorSlot);
	AddTexture("wallpaper", PT::Wallpaper(), WallSlot);
	AddTexture("rug", PT::Rug(), RugSlot);
	AddTexture("beach-ball", PT::BeachBall(), BallSlot);
	AddTexture("toy-block", PT::ToyBlock(128, glm::vec3(1.0f)), BlockSlot);
	AddTexture("night-sky", PT::NightSky(), StarsSlot);
	AddTexture("lunar-surface", PT::Moon(), MoonSlot);
	AddTexture("woven-cotton", PT::Fabric(0), FabricSlot);
	AddTexture("denim", PT::Fabric(1), DenimSlot);
	AddTexture("worn-leather", PT::Fabric(2), LeatherSlot);
	AddTexture("shirt-plaid", PT::Fabric(3), PlaidSlot);
	AddTexture("cow-print", PT::Fabric(4), CowSlot);
	AddTexture("siding", PT::Siding());
	AddTexture("shingles", PT::Shingles());
	AddTexture("brick", PT::Brick());
	AddTexture("grass", PT::Grass());
	AddTexture("book-spines", PT::BookSpines(), BookSlot);
	AddTexture("pickets", PT::Pickets(), PicketSlot);
	AddTexture("window-pane", PT::WindowPane());
	AddTexture("flower-bed", PT::FlowerBed());

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
	named[name] = tex;
	if (slot >= 0)
		slotTextures[static_cast<size_t>(slot)] = tex;
	return tex;
}

const Texture* Assets::Named(const std::string& name) const
{
	auto it = named.find(name);
	return it == named.end() ? nullptr : it->second;
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
