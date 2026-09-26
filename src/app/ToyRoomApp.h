#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Application.h"
#include "render/Assets.h"
#include "render/RayTracer.h"
#include "render/RenderSettings.h"
#include "render/Renderer.h"
#include "scene/Camera.h"
#include "scene/Light.h"
#include "scene/SceneNode.h"
#include "world/Environment.h"
#include "world/StoryDirector.h"

class Character;
class Humanoid;
class Buzz;
class Bullseye;
class RCCar;

// Everything the user can select (number keys or mouse click) and inspect.
struct Selectable {
	std::string name;
	SceneNode* node = nullptr;
	Character* character = nullptr; // null for props (ball, lamp, ghost)
	int key = 0;                    // number key that selects it (0 = mouse only)
	float focusHeight = 1.0f;       // camera target height above the node origin
	float focusDistance = 5.0f;     // orbit distance used by F (focus)
	Transform initial;              // for Backspace (reset)
};

enum class EditOp : int { Translate = 0, Rotate, Scale, Shear };

// Command-line options for scripted runs (used to produce the screenshots in the docs):
//   --hour 21.5  --select 2  --focus  --mount  --raytrace  --shading 0..3  --wireframe
//   --normals  --cam x,y,z,tx,ty,tz  --capture out.bmp  --frames 90
struct LaunchOptions {
	float hour = -1.0f;
	int select = -1;
	bool focus = false;
	bool mount = false;
	bool rayTrace = false;
	int shading = -1;
	bool wireframe = false;
	bool normals = false;
	bool pauseClock = false;
	bool hasCamera = false;
	glm::vec3 cameraPos{ 0.0f }, cameraTarget{ 0.0f };
	float orbitYaw = 1e9f, orbitPitch = 1e9f, orbitDistance = -1.0f;
	std::string capture;   // save a screenshot here after 'frames' frames, then quit
	int frames = 90;

	static LaunchOptions Parse(int argc, char* argv[]);
};

class ToyRoomApp : public Application {
public:
	explicit ToyRoomApp(const LaunchOptions& options = {});
	~ToyRoomApp() override;

protected:
	void OnUpdate(float dt) override;
	void OnRender() override;

private:
	void BuildScene();
	void AddSelectable(const std::string& name, SceneNode* node, Character* character, int key, float focusHeight, float focusDistance);

	// Input handling
	void HandleGlobalKeys();
	void HandleSelection();
	void HandleCamera(float dt);
	void HandleObjectControl(float dt);
	void HandleEditMode(float dt);

	void Select(int id);
	Selectable* Selected();
	Character* DrivenCharacter(); // character WASD controls (Jessie mounted -> Bullseye)
	int Pick(const glm::vec2& mouse) const;

	// Jessie <-> Bullseye
	void ToggleMount();
	void Mount();
	void Dismount();

	void UpdateLights();
	void UpdateTitle();
	void PrintHelp() const;
	void DumpSelectedGeometry(bool full) const;
	void SaveScreenshot(const std::string& path = {});
	void ApplyLaunchOptions();

	Assets assets;
	Renderer renderer;
	RayTracer rayTracer;
	RenderSettings settings;
	Camera camera;
	std::unique_ptr<SceneNode> scene;
	std::vector<Light> lights;
	Environment environment;
	StoryDirector story;

	std::vector<std::unique_ptr<Character>> characters;
	Humanoid* woody = nullptr;
	Humanoid* jessie = nullptr;
	Bullseye* bullseye = nullptr;
	Buzz* buzz = nullptr;
	RCCar* car = nullptr;
	bool jessieMounted = false;

	std::vector<Selectable> selectables;
	int selectedId = -1;
	int ballId = -1, lampId = -1;

	bool editMode = false;
	EditOp editOp = EditOp::Translate;

	glm::vec2 mouseDownPos{ 0.0f };
	float mouseDragDistance = 0.0f;
	float titleTimer = 0.0f;
	int screenshotCounter = 0;
	bool wasMoving = false;
	LaunchOptions launch;
	int frameCounter = 0;
};
