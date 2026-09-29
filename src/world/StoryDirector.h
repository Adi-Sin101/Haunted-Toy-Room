#pragma once
#include <functional>
#include <string>
#include <vector>
#include <glm/glm.hpp>
class Character; class Humanoid; class Bullseye; class Buzz; class RCCar;
class SceneNode; class Environment; class PhysicsWorld;

// Selection pauses the film. An explicitly activated car can finish its route in manual mode.
class StoryDirector {
public:
 enum class Scene { Discovery, Departure, ClearPath, ReachCar, ActivateCar, ReturnHome, Morning, End };
 void Init(Humanoid*, Humanoid*, Bullseye*, Buzz*, RCCar*, SceneNode*, Environment*, PhysicsWorld*,
  std::function<void()> mount, std::function<void()> dismount);
 void Restart(bool mounted);
 void Update(float dt, bool mounted, bool interact);
 void Pause();
 bool CarAutopilot() const { return activated && phase==Scene::ActivateCar; }
 std::string Title() const;
 std::string Caption() const;
 Scene CurrentScene() const { return phase; }
 float SceneTime() const { return elapsed; }
 bool enabled = true;
private:
 struct Route { Character* actor; std::vector<glm::vec3> points; size_t next = 0; };
 bool Move(Character*, float dt);
 void Enter(Scene);
 void Routes(std::vector<Route>);
 Humanoid* woody=nullptr; Humanoid* jessie=nullptr;
 Bullseye* horse=nullptr; Buzz* buzz=nullptr; RCCar* car=nullptr;
 SceneNode* obstacle=nullptr; Environment* environment=nullptr; PhysicsWorld* physics=nullptr;
 std::function<void()> mount, dismount;
 std::vector<Route> routes;
 Scene phase=Scene::Discovery;
 float elapsed=0;
 bool activated=false, dismountRequested=false, obstacleHit=false;
 glm::vec3 obstacleHome{0};
};
