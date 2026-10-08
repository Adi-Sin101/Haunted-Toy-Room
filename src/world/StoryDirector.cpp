#include "StoryDirector.h"
#include "Room.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include "characters/Cat.h"
#include "render/Assets.h"
#include "scene/SceneNode.h"
#include "scene/Material.h"
#include <utility>
#include <vector>

#include "characters/Bullseye.h"
#include "characters/Character.h"
#include "characters/Humanoid.h"
#include "characters/RCCar.h"
#include "world/PhysicsWorld.h"

void StoryDirector::Init(Humanoid* w, Humanoid* j, Bullseye* h, Buzz* b, RCCar* c,
	PhysicsWorld* p, std::function<void()> d)
{
	woody = w;
	jessie = j;
	horse = h;
	buzz = b;
	car = c;
	physics = p;
	dismount = std::move(d);
	Restart(false);
}

void StoryDirector::Pause()
{
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz, car}) {
		if (actor) actor->Stop();
	}
	if (buzz && buzz->LaserOn()) buzz->Special();
}

void StoryDirector::Restart(bool mounted)
{
	if (mounted && dismount) dismount();
	for (Character* actor : std::vector<Character*>{woody, jessie, horse, buzz, car}) {
		if (!actor) continue;
		actor->StartTransition(actor->HomePosition(), actor->HomeHeading(), 0.01f);
		actor->Animate(0.02f, 0);
		actor->RestoreRestPose();
	}
	if (physics) physics->ResetBlocks();
	if (car) car->SetHeadlights(false);
	if (buzz && buzz->LaserOn()) buzz->Special();
    switches={}; ridingAtSwitch=doorBroken=finalTriggered=false;
    elapsed=firing=chaseTime=0; escapeRoutes.clear(); controlled=attached=nullptr;
    if (doorLeaf) doorLeaf->visible=doorLeaf->solid=true;
    if (lock) lock->visible=true;
    if (house.frontDoor) { house.frontDoor->visible=true; house.frontDoor->local.rotation.y=0; }
    for (SceneNode* n:debris) n->visible=false;
    if (toyBarrier) { toyBarrier->visible=true; toyBarrier->local.position.y=0; }
    if (buzzBarrier) buzzBarrier->visible=true;
    for (SceneNode* n:switchHandles) if (n) n->local.rotation.z=0;
    if (physics) physics->SetExteriorAccess(false);
    enabled = true;
    Enter(GameplayState::PROLOGUE);
}

void StoryDirector::BeginGameplay()
{
	Advance(Transition::ARRIVAL_COMPLETE);
}

bool StoryDirector::Advance(Transition transition)
{
	GameplayState next = state;
	switch (transition) {
	case Transition::ARRIVAL_COMPLETE:
		if (state == GameplayState::PROLOGUE) next = GameplayState::PUZZLE;
		break;
	case Transition::PUZZLE_SOLVED:
		if (state == GameplayState::PUZZLE) next = GameplayState::TOY_RESCUE;
		break;
	case Transition::TOYS_FREED:
		if (state == GameplayState::TOY_RESCUE) next = GameplayState::BUZZ_RESCUE;
		break;
	case Transition::BUZZ_FREED:
		if (state == GameplayState::BUZZ_RESCUE) next = GameplayState::FINAL_ESCAPE;
		break;
	case Transition::ESCAPE_COMPLETE:
		if (state == GameplayState::FINAL_ESCAPE) next = GameplayState::WIN;
		break;
	}
	if (next == state) return false;
	Enter(next);
	return true;
}

void StoryDirector::Enter(GameplayState next)
{
	state = next; elapsed=0;
    if (next==GameplayState::FINAL_ESCAPE) BeginEscape();
	std::cout << "GAMEPLAY " << Title() << "\n";
}

std::string StoryDirector::Title() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "STAGE 0 / PROLOGUE - ARRIVAL";
	case GameplayState::PUZZLE: return "STAGE 1 / ENTRANCE - PUZZLE";
	case GameplayState::TOY_RESCUE: return "STAGE 2 / TOY RESCUE";
	case GameplayState::BUZZ_RESCUE: return "STAGE 3 / BUZZ ROOM";
	case GameplayState::FINAL_ESCAPE: return "STAGE 4 / FINAL ESCAPE";
	case GameplayState::WIN: return "WIN / ENDING";
	}
	return {};
}

std::string StoryDirector::Objective() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "The entrance note: the toys need help";
	case GameplayState::PUZZLE: return "Find the code: 3 clues";
	case GameplayState::TOY_RESCUE: return switches[0] && switches[1] ? "Jessie: ride, dismount on platform, Enter high switch" : "Penny: Enter at both low red switches";
	case GameplayState::BUZZ_RESCUE: return "Penny: Enter at the rear red release";
	case GameplayState::FINAL_ESCAPE: return doorBroken ? "Take Penny outside; wait for every rescued toy" : "Downstairs: Enter at the sealed front door";
	case GameplayState::WIN: return "THE TOYS ARE SAFE\nYOU ESCAPED";
	}
	return {};
}

std::string StoryDirector::Caption() const
{
	switch (state) {
	case GameplayState::PROLOGUE: return "Penny arrives at the abandoned house as night falls.";
	case GameplayState::PUZZLE: return "Inspect the toy train, old clock, and colored blocks, then enter their numbers at the keypad.";
	case GameplayState::TOY_RESCUE: return "Woody, Jessie and Bullseye are trapped inside the Toy Room.";
	case GameplayState::BUZZ_RESCUE: return "Woody points Penny toward the room where Buzz is trapped.";
	case GameplayState::FINAL_ESCAPE: return "The house is hostile. Get the rescued toys back to the main entrance.";
	case GameplayState::WIN: return "The toys escaped. The house falls dark behind them.";
	}
	return {};
}


namespace {
const std::array<glm::vec3,3> SwitchPositions{{{7.1f,0.75f,2.0f},{7.1f,0.75f,5.8f},{2.5f,3.25f,4.5f}}};
const glm::vec3 BuzzRelease{2.8f,0.8f,-3.0f};
float DistanceXZ(const glm::vec3& a,const glm::vec3& b) { return glm::length(glm::vec2(a.x-b.x,a.z-b.z)); }
}

void StoryDirector::Build(SceneNode& scene,Assets& assets,const HouseRig& rig,Cat* cat,SceneNode* visitor)
{
    world=&scene; house=rig; penny=cat; ghost=visitor;
    auto& iron=assets.Mat("rescue-iron",{0.10f,0.13f,0.16f},0.65f,64);
    iron.reflectivity=0.10f;
    auto& switchCase=assets.Mat("rescue-switch-case",{0.28f,0.22f,0.14f},0.2f,32);
    auto& red=assets.Mat("rescue-switch-red",{0.85f,0.10f,0.05f},0.4f,40);
    auto& glow=assets.Mat("buzz-energy-barrier",{0.15f,0.70f,0.95f},0.6f,60);
    glow.opacity=0.45f; glow.emissive={0.03f,0.20f,0.35f};
    toyBarrier=scene.AddChild("ToyRescueBarrier");
    for (int i=0;i<9;++i) {
        auto* bar=toyBarrier->AddShape("IronBar",&assets.Cylinder(),&iron,{4.4f,1.4f,0.2f+i*0.8f},{0.10f,2.8f,0.10f});
        bar->solid=true;
    }
    // One continuous collision proxy keeps small actors from slipping through bars.
    auto* gate=toyBarrier->AddShape("GateRail",&assets.Cube(),&iron,{4.4f,1.4f,3.4f},{0.10f,0.12f,6.8f});
    gate->solid=true;
    for (int i=0;i<3;++i) {
        auto* joint=scene.AddChild("RescueSwitch"+std::to_string(i+1));joint->local.position=SwitchPositions[i];
        joint->AddShape("SwitchBackplate",&assets.Cube(),&switchCase,{0,0,0},{0.50f,0.60f,0.16f});
        switchHandles[i]=joint->AddShape("SwitchLever",&assets.Cylinder(),&red,{0,0.04f,0.15f},{0.07f,0.33f,0.07f},{0,0,20});
    }
    auto* platform=scene.AddShape("HighSwitchPlatform",&assets.Cube(),&switchCase,{2.5f,0.65f,4.5f},{2.0f,1.30f,1.6f});
    platform->solid=true;
    // Partition the existing rear corner, retaining the room's floor and lighting.
    auto& wall=assets.Mat("buzz-room-wall",{0.35f,0.39f,0.43f},0.05f,12);
    scene.AddShape("BuzzRoomPartitionLeft",&assets.Cube(),&wall,{-2.7f,1.6f,-4.2f},{1.4f,3.2f,0.18f})->solid=true;
    scene.AddShape("BuzzRoomPartitionRight",&assets.Cube(),&wall,{2.7f,1.6f,-4.2f},{1.4f,3.2f,0.18f})->solid=true;
    buzzBarrier=scene.AddShape("BuzzEnergyBarrier",&assets.Cube(),&glow,{0,1.55f,-4.2f},{4.0f,3.1f,0.06f});buzzBarrier->solid=true;
    scene.AddShape("BuzzReleaseSwitch",&assets.Cube(),&red,BuzzRelease,{0.45f,0.5f,0.2f});
    buzz->Root()->local.position={0,0,-6.6f};buzz->SaveHome();
    doorLeaf=house.frontDoor->Find("FrontDoorLeaf");
    lock=house.frontDoor->AddChild("EntrancePadlock");lock->local.position={1.20f,1.20f,-0.11f};
    lock->AddShape("LockBody",&assets.Cube(),&iron,{0,0,0},{0.24f,0.28f,0.12f});
    for (float x:{-0.075f,0.075f}) lock->AddShape("ShackleUpright",&assets.Cylinder(),&iron,{x,0.20f,0},{0.045f,0.16f,0.045f});
    lock->AddShape("ShackleCrown",&assets.Cylinder(),&iron,{0,0.28f,0},{0.045f,0.15f,0.045f},{0,0,90});
    // Six preallocated boards use the existing fixed-step rigid-body solver when broken.
    auto& wood=assets.Mat("broken-door-wood",{0.48f,0.20f,0.12f},0.25f,24);
    wood.texture=assets.SlotTexture(Assets::FloorSlot);wood.rtTextureSlot=Assets::FloorSlot;
    for (int i=0;i<6;++i) {
        auto* board=scene.AddShape("DoorDebris"+std::to_string(i),&assets.Cube(),&wood,
            {11.4f+(i%3)*0.5f,RoomSize::Ground+0.8f+(i/3)*1.3f,9.3f},{0.38f,1.1f,0.08f});
        board->visible=false;debris.push_back(board);
    }
    auto& paper=assets.Mat("rescue-note-paper",{0.88f,0.82f,0.61f},0.05f,10);
    scene.AddShape("EntranceRescueNote",&assets.Cube(),&paper,{11.0f,RoomSize::Ground+1.2f,9.38f},{0.65f,0.45f,0.025f});
}

glm::vec3 StoryDirector::DismountTarget(const glm::vec3& normal,bool mounted) const
{
    if (state==GameplayState::TOY_RESCUE && mounted && switches[0] && switches[1] && DistanceXZ(horse->Root()->WorldPosition(),SwitchPositions[2])<2.0f)
        return {2.5f,1.30f,4.5f};
    return normal;
}

std::string StoryDirector::Interact(Character* actor,bool mounted)
{
    if (!actor || state==GameplayState::PROLOGUE || state==GameplayState::PUZZLE) return {};
    const auto p=actor->Root()->WorldPosition();
    if (state==GameplayState::TOY_RESCUE) {
        for (int i=0;i<2;++i) if (actor==penny && DistanceXZ(p,SwitchPositions[i])<1.7f) {
            switches[i]=true; switchHandles[i]->local.rotation.z=-45;
            return switches[0] && switches[1] ? "Two switches active. Jessie: mount Bullseye, ride to the high platform, dismount and press Enter." : "Switch active. Find the other low switch.";
        }
        if (actor==jessie && !mounted && ridingAtSwitch && p.y>1.0f && DistanceXZ(p,SwitchPositions[2])<1.5f && !jessie->InTransition()) {
            switches[2]=true;switchHandles[2]->local.rotation.z=-45;toyBarrier->visible=false;
            Advance(Transition::TOYS_FREED);return "Toys freed. Select Penny and find Buzz's release switch in the rear corner.";
        }
        return "Penny activates two low switches; Jessie reaches the high switch using Bullseye and the platform.";
    }
    if (state==GameplayState::BUZZ_RESCUE) {
        if (actor!=penny || DistanceXZ(p,BuzzRelease)>1.8f) return "Penny: press Enter near the red release switch outside Buzz's barrier.";
        buzzBarrier->visible=false;buzzBarrier->solid=false;
        Advance(Transition::BUZZ_FREED);return "Buzz freed. Return downstairs to the front door; the haunted toy is chasing Penny.";
    }
    if (state==GameplayState::FINAL_ESCAPE && !doorBroken) {
        if (actor!=penny || p.y>RoomSize::Slab || DistanceXZ(p,{12,0,9.2f})>2.6f) return "Penny: return downstairs and press Enter at the sealed entrance.";
        finalTriggered=true;return "Buzz is flying to the entrance. Keep the laser path clear; 0 releases Buzz if selected.";
    }
    return {};
}

void StoryDirector::BeginEscape()
{
    escapeRoutes.clear();
    for (Character* actor:{static_cast<Character*>(woody),static_cast<Character*>(jessie),static_cast<Character*>(horse),static_cast<Character*>(buzz)}) {
        actor->clampToRoom=false;
        escapeRoutes.push_back({actor,0,actor->Root()->WorldPosition(),false});
    }
    if (jessie->Seated() && dismount) dismount();
    if (ghost) ghost->visible=true;
}

void StoryDirector::BreakDoor()
{
    doorBroken=true;house.frontDoor->visible=false;doorLeaf->solid=false;
    if (lock) lock->visible=false;
    physics->SetExteriorAccess(true);
    for (size_t i=0;i<debris.size();++i) {
        debris[i]->visible=true;
        physics->PushBlock(debris[i],{-4.0f-static_cast<float>(i%3),1.0f,4.5f},{75.0f,15.0f,65.0f});
    }
    if (buzz->LaserOn() && !Controls(buzz)) buzz->Special();
    std::cout<<"ESCAPE real laser broke entrance door\n";
}

void StoryDirector::Update(float dt,bool mounted)
{
    if (!enabled || state==GameplayState::PROLOGUE || state==GameplayState::WIN) return;
    elapsed+=dt;
    if (state==GameplayState::TOY_RESCUE) {
        if (mounted && switches[0] && switches[1] && DistanceXZ(horse->Root()->WorldPosition(),SwitchPositions[2])<2.0f) ridingAtSwitch=true;
        if (switches[0] && switches[1]) toyBarrier->local.position.y=3.0f;
    }
    if (state!=GameplayState::FINAL_ESCAPE) return;
    chaseTime+=dt;
    if (ghost) {
        const auto goal=penny->Root()->WorldPosition()+glm::vec3(0,0.8f,0);
        glm::vec3 d=goal-ghost->local.position;
        float length=glm::length(d);
        if (length>1.2f) ghost->local.position+=d/length*std::min(length-1.2f,dt*1.6f);
        ghost->local.rotation.z=std::sin(chaseTime*2.0f)*6.0f;
    }
    for (size_t i=0;i<escapeRoutes.size();++i) {
        auto& route=escapeRoutes[i]; auto* actor=route.actor;
        if (route.done || elapsed<i*2.0f) continue;
        std::array<glm::vec3,12> points{{{3,0,0},{8,0,0},{8,0,4},{12,0,4},{15,0,2},{15,RoomSize::Ground,-7.5f},
            {12,RoomSize::Ground,-7.5f},{12,RoomSize::Ground,6.0f-i*1.7f},{12,RoomSize::Ground,9.5f},
            {14.0f+i*0.6f,RoomSize::Ground,12.0f+i*0.8f},{7.0f+i*2.1f,RoomSize::Ground,15},{7.0f+i*2.1f,RoomSize::Ground,16}}};
        if (route.next==8 && !doorBroken) {
            if (actor==buzz && finalTriggered && !Controls(buzz)) {
                const glm::vec3 flight{12,RoomSize::Ground+2.6f,7.0f};
                const auto takeoff=glm::vec3(buzz->Root()->local.position.x,flight.y,buzz->Root()->local.position.z);
                if (!buzz->FollowWaypoint(takeoff,dt,2.5f)) continue;
                if (buzz->FollowWaypoint(flight,dt,2.5f)) {
                    buzz->AimAt({12,RoomSize::Ground+1.4f,9.2f});
                    if (!buzz->LaserOn()) buzz->Special();

                }
            }
            continue;
        }
        if (Controls(actor)) {
            // Only a virtual cursor progresses; no user-owned transform or action is written.
            const auto delta=points[route.next]-route.progress; const float d=glm::length(delta);
            if (d<0.06f) ++route.next; else route.progress+=delta/d*std::min(d,dt*2.0f);
        } else {
            if ((actor->CanFly() ? glm::distance(actor->Root()->local.position,points[route.next]) : DistanceXZ(actor->Root()->local.position,points[route.next]))<0.18f || actor->FollowWaypoint(points[route.next],dt,2.0f)) ++route.next;
            route.progress=actor->Root()->local.position;
            if (!actor->CanFly()) actor->Root()->local.position.y=PhysicsWorld::FloorHeight(actor->Root()->local.position);
        }
        if (route.next>=points.size()) { route.done=true; if (!Controls(actor)) actor->Stop(); }
    }
    if (finalTriggered && !doorBroken) {
        if (buzz->LaserOn() && physics->LastLaserHit()==doorLeaf) firing+=dt; else firing=0;
        if (firing>0.65f) BreakDoor();
    }
    const auto cat=penny->Root()->WorldPosition();
    const bool all=std::all_of(escapeRoutes.begin(),escapeRoutes.end(),[](const auto& r){return r.done && r.actor->Root()->WorldPosition().z>13 && r.actor->Root()->WorldPosition().y<RoomSize::Slab;});
    if (doorBroken && cat.z>13 && cat.y<RoomSize::Slab && all) {
        if (ghost) ghost->visible=false;
        for (auto& route:escapeRoutes) if (!Controls(route.actor)) { const auto placement=route.actor->Root()->local; route.actor->RestoreRestPose(); route.actor->Root()->local=placement; }
        Advance(Transition::ESCAPE_COMPLETE);
    }
}

void StoryDirector::SetControlled(Character* actor, Character* passenger)
{
    if (controlled==actor && attached==passenger) return;
    for (auto& route:escapeRoutes) {
        if (Controls(route.actor) && route.actor!=actor && route.actor!=passenger) {
            const auto position=route.actor->Root()->local.position;
            // Rejoin the connected path at the actor's actual floor, without teleporting.
            route.next=position.z>13 && position.y<RoomSize::Slab ? 10 :
                position.y<RoomSize::Slab ? (position.z<-6 ? 6 : 7) :
                position.x>=RoomSize::StairLeft ? 5 : position.x>10 ? 4 : 0;
            route.progress=position;
            route.done=false;
        }
    }
    controlled=actor; attached=passenger;
}
