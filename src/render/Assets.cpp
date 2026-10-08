#include "Assets.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <stdexcept>

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
	AddTexture("siding", PT::Siding(), SidingSlot);
	AddTexture("shingles", PT::Shingles(), ShingleSlot);
	AddTexture("brick", PT::Brick(), BrickSlot);
	AddTexture("grass", PT::Grass(), GrassSlot);
	AddTexture("book-spines", PT::BookSpines(), BookSlot);
	AddTexture("pickets", PT::Pickets(), PicketSlot);
	AddTexture("window-pane", PT::WindowPane(), WindowSlot);
	AddTexture("flower-bed", PT::FlowerBed(), FlowerSlot);
	AddTexture("chest-paint", PT::ChestPaint(), ChestSlot);
	AddTexture("star-wallpaper", PT::StarWallpaper(), StarWallSlot);
	AddTexture("star-quilt", PT::StarQuilt(), QuiltSlot);
	AddTexture("star-decal", PT::StarDecal(), StarDecalSlot);
	AddTexture("football", PT::Soccer(), SoccerSlot);

	// The poster is a real image file read by our own BMP loader.
	const std::string posterPath = Paths::resolve("assets/textures/poster.bmp").string();
	if (auto poster = Bmp::Load(posterPath)) {
		AddTexture("poster", *poster, PosterSlot);
		std::cout << "Loaded " << posterPath << " (" << poster->width << "x" << poster->height << ")\n";
	}
	else {
		AddTexture("poster", PT::PosterFallback(), PosterSlot);
	}
	const std::string clockPath=Paths::resolve("assets/textures/clock.bmp").string();
	if (auto clock=Bmp::Load(clockPath)) AddTexture("toy-story-clock",*clock,ClockSlot);
	else AddTexture("toy-story-clock",PT::White(),ClockSlot);
	// All traced materials share one array sampler, below GL 3.3's sampler limit.
	// Normalised UVs preserve aspect-independent mapping; bilinear resampling keeps fine patterns.
	constexpr int side = 512;
	glGenTextures(1, &surfaceArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, surfaceArray);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, side, side, SlotCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	Image layer(side, side);
	for (const auto& [name, slot] : namedSlots) {
		const Image& source = sourceImages.at(name);
		for (int y = 0; y < side; ++y) for (int x = 0; x < side; ++x) {
			const float sx = (x + 0.5f) * source.width / side - 0.5f;
			const float sy = (y + 0.5f) * source.height / side - 0.5f;
			const int ix = static_cast<int>(std::floor(sx)), iy = static_cast<int>(std::floor(sy));
			const float fx = sx - ix, fy = sy - iy;
			for (int channel = 0; channel < 4; ++channel) {
				auto pixel = [&](int px, int py) { return source.pixels[(static_cast<size_t>((py + source.height) % source.height) * source.width + (px + source.width) % source.width) * 4 + channel]; };
				const float bottom = pixel(ix, iy) * (1 - fx) + pixel(ix + 1, iy) * fx;
				const float top = pixel(ix, iy + 1) * (1 - fx) + pixel(ix + 1, iy + 1) * fx;
				layer.pixels[(static_cast<size_t>(y) * side + x) * 4 + channel] = static_cast<std::uint8_t>(bottom * (1 - fy) + top * fy + 0.5f);
			}
		}
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, slot, side, side, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer.pixels.data());
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

Assets::~Assets() { if (surfaceArray) glDeleteTextures(1, &surfaceArray); }

int Assets::NamedSlot(const std::string& name) const
{
	auto it = namedSlots.find(name);
	return it == namedSlots.end() ? -1 : it->second;
}

void Assets::ExportTextures(const std::string& directory) const
{
	std::filesystem::create_directories(directory);
	std::ofstream manifest(std::filesystem::path(directory) / "textures.csv");
	manifest << "name,width,height,ray_layer\n";
	for (const auto& [name, image] : sourceImages) {
		manifest << name << ',' << image.width << ',' << image.height << ',' << NamedSlot(name) << '\n';
		std::ofstream rgba(std::filesystem::path(directory) / (name + ".rgba"), std::ios::binary);
		rgba.write(reinterpret_cast<const char*>(image.pixels.data()), static_cast<std::streamsize>(image.pixels.size()));
		if (!rgba) throw std::runtime_error("RGBA texture export failed: " + name);
		if (!Bmp::Save((std::filesystem::path(directory) / (name + ".bmp")).string(), image))
			throw std::runtime_error("Texture export failed: " + name);
	}
}

const Texture* Assets::AddTexture(const std::string& name, const Image& image, int slot, bool nearest)
{
	textures.push_back(std::make_unique<Texture>(name, image, nearest));
	const Texture* tex = textures.back().get();
	named[name] = tex;
	sourceImages[name] = image;
	if (slot >= 0) namedSlots[name] = slot;
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
