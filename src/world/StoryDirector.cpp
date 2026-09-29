#include "StoryDirector.h"
#include <algorithm>
#include <iostream>
#include "characters/Humanoid.h"
#include "characters/Bullseye.h"
#include "characters/RCCar.h"
#include "scene/SceneNode.h"
#include "world/Environment.h"
#include "world/PhysicsWorld.h"

void StoryDirector::Init(Humanoid* w, Humanoid* j, Bullseye* h, Buzz* b, RCCar* c,
 SceneNode* o, Environment* e, PhysicsWorld* p, std::function<void()> m, std::function<void()> d)
{
 woody=w; jessie=j; horse=h; buzz=b; car=c; obstacle=o; environment=e; physics=p;
 mount=std::move(m); dismount=std::move(d); obstacleHome=o->local.position;
 Restart(false);
}
void StoryDirector::Pause()
{
 for (Character* c : std::vector<Character*>{woody,jessie,horse,buzz,car}) c->Stop();
 if (buzz->LaserOn()) buzz->Special();
}
void StoryDirector::Restart(bool mounted)
{
 if (mounted) dismount();
 for (Character* c : std::vector<Character*>{woody,jessie,horse,buzz,car}) {
  c->StartTransition(c->HomePosition(),c->HomeHeading(),0.01f);
  c->Animate(0.02f,0);
  c->RestoreRestPose();
 }
 physics->ResetBlocks(); obstacle->visible=true;
 car->Root()->local.position={15.4f,0,4}; car->Root()->local.rotation.y=-90; car->SetHeadlights(false);
 if (buzz->LaserOn()) buzz->Special();
 environment->hour=0; environment->paused=false; environment->lampManual=false; environment->lampPower=true;
 environment->StopBall(); enabled=true; activated=false; dismountRequested=false; obstacleHit=false;
 Enter(Scene::Discovery);
}
void StoryDirector::Routes(std::vector<Route> next) { routes=std::move(next); }
bool StoryDirector::Move(Character* c,float dt)
{
 for (Route& r : routes) if (r.actor==c) {
  if (r.next==r.points.size()) { c->Stop(); return true; }
  if (c->FollowWaypoint(r.points[r.next],dt,c==car ? 2.0f : 1.8f)) ++r.next;
  return r.next==r.points.size();
 }
 return true;
}
void StoryDirector::Enter(Scene next)
{
 phase=next; elapsed=0; routes.clear();
 std::cout << "MISSION " << Title() << "\n";
 switch (phase) {
 case Scene::Departure:
  Routes({{woody,{{-3,0,1.9f},{-1.5f,0,1.9f},{-1.5f,0,2.7f},{6.5f,0,2.7f}}},{jessie,{{-0.8f,0,3.5f},{3.5f,0,3.0f},{3.5f,0,0.3f}}},
   {buzz,{{0.4f,1.7f,-3},{6.2f,1.7f,-3},{6.2f,1.7f,5.8f}}}}); break;
 case Scene::ReachCar:
  Routes({{woody,{{11.4f,0,2.7f},{14,0,2.7f}}},
   {horse,{{2.2f,0,5.6f},{11.5f,0,5.6f},{14,0,5.6f}}},
   {buzz,{{6.2f,2.4f,2.7f},{12,2.4f,2.7f},{15,2.4f,2.7f}}}}); break;
 case Scene::ActivateCar:
  Routes({{car,{{11.5f,0,4},{8,0,4},{8,0,-1.8f},car->HomePosition()}}}); break;
 case Scene::ReturnHome:
  Routes({{horse,{{11.5f,0,5.6f},{2.2f,0,5.6f},horse->HomePosition()}},
   {woody,{{11.4f,0,2.7f},{-1.5f,0,2.7f},{-1.5f,0,0.5f},woody->HomePosition()}},
   {buzz,{{12,2.4f,2.7f},{6.2f,2.4f,2.7f},{0.4f,2.4f,-3},{0.4f,1.7f,-1.8f},buzz->HomePosition()}},
   {jessie,{jessie->HomePosition()}}}); break;
 default: break;
 }
}
void StoryDirector::Update(float dt,bool mounted,bool interact)
{
 if (dt<=0) return;
 if (interact && phase==Scene::ActivateCar) activated=true;
 if (!enabled) {
  if (CarAutopilot()) { car->SetHeadlights(true); if (Move(car,dt)) Enter(Scene::ReturnHome); }
  return;
 }
 elapsed+=dt;
 switch (phase) {
 case Scene::Discovery:
  horse->Root()->local.rotation.y=0;
  environment->hour=0; woody->TurnTowardsHeading(75,dt);
  if (elapsed>4) Enter(Scene::Departure); break;
 case Scene::Departure: {
  const bool w=Move(woody,dt),b=Move(buzz,dt);
  bool j=mounted;
  if (!mounted && Move(jessie,dt)) { mount(); j=true; }
  if (w && b && j && !jessie->InTransition()) Enter(Scene::ClearPath);
  break; }
 case Scene::ClearPath:
  buzz->AimAt(obstacle->WorldPosition());
  if (!buzz->LaserOn()) buzz->Special();
  if (physics->LastLaserHit()==obstacle) obstacleHit=true;
  if (obstacleHit && glm::distance(obstacle->local.position,obstacleHome)>0.7f) {
   obstacle->visible=false; physics->StopBlock(obstacle); buzz->Special(); Enter(Scene::ReachCar);
  } break;
 case Scene::ReachCar: {
  const bool b=Move(buzz,dt),w=Move(woody,dt),h=Move(horse,dt);
  if (b && w && h) Enter(Scene::ActivateCar); break; }
 case Scene::ActivateCar:
  // Enter activates immediately; unattended playback performs the interaction after a beat.
  if (elapsed>3) activated=true;
  if (activated) { car->SetHeadlights(true); if (Move(car,dt)) Enter(Scene::ReturnHome); }
  break;
 case Scene::ReturnHome: {
  const bool h=Move(horse,dt),w=Move(woody,dt),b=Move(buzz,dt);
  if (h && mounted && !dismountRequested) { dismount(); dismountRequested=true; }
  const bool j=!mounted && !jessie->InTransition() && Move(jessie,dt);
  if (h && w && b && j) {
   bool facing=true;
   for (Character* c : std::vector<Character*>{woody,jessie,horse,buzz}) facing=c->TurnTowardsHeading(c->HomeHeading(),dt) && facing;
   if (facing) Enter(Scene::Morning);
  } break; }
 case Scene::Morning:
  environment->hour=5+std::min(1.0f,elapsed/12)*3;
  environment->lampManual=true; environment->lampPower=false;
  for (Character* c : std::vector<Character*>{woody,jessie,horse,buzz,car}) c->Stop();
  if (elapsed>=12) Enter(Scene::End); break;
 case Scene::End: environment->hour=8; break;
 }
}
std::string StoryDirector::Title() const
{
 if (CarAutopilot()) return "5 / Driving Home";
 const char* titles[]={"1 / The Discovery","2 / Moving Outside","3 / Buzz Clears the Path",
  "4 / Reaching the Car","5 / Activating the Car","6 / Returning Home","7 / Morning","The End"};
 return titles[static_cast<int>(phase)];
}
std::string StoryDirector::Caption() const
{
 if (CarAutopilot()) return "Headlights on. The car follows its route back into the toy room.";
 const char* captions[]={"Woody spots the lost car beyond the blocked doorway.",
  "Woody leads. Jessie mounts Bullseye. Buzz takes flight.",
  "Buzz aims his laser and pushes the obstacle out of the way.",
  "The friends cross the doorway to rescue the car.",
  "Enter activates the car. Its lights and wheels lead it home.",
  "Woody returns, Jessie rides back, and Buzz lands.",
  "Dawn fills the room. The lamp goes dark; the toys stand still.",
  "The Midnight Mission is complete. Shift+N replays the story."};
 return captions[static_cast<int>(phase)];
}
