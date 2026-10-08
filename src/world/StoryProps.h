#pragma once

#include <glm/glm.hpp>

class Assets;
class SceneNode;
struct Material;

// Handles to the story's moving props: the toy chest in the Toy Room, and Buzz's bedroom on the
// ground floor with its ordinary door and the wardrobe Buzz is trapped in.
struct StoryRig {
	// Toy chest (front faces +Z). The lid joint sits on the back top edge: rotation.x 0 (closed)
	// .. -105 (open). The button joint scales down in Y while it is pressed; the wind-up key spins in X.
	SceneNode* chest = nullptr;
	SceneNode* chestLid = nullptr;
	SceneNode* chestButton = nullptr;
	SceneNode* chestKey = nullptr;
	Material* buttonGlow = nullptr;
	glm::vec3 chestCentre{0.0f};

	// Buzz's room door: hinge on the corridor wall, rotation.y 0 (closed) .. 95 (open into the room).
	SceneNode* buzzDoor = nullptr;
	// Wardrobe doors (front faces +X): left hinge rotation.y 0 .. -100, right hinge 0 .. +100.
	SceneNode* wardrobeLeft = nullptr;
	SceneNode* wardrobeRight = nullptr;
	Material* wardrobeGlow = nullptr;   // light leaking through the door gap while Buzz is inside
	glm::vec3 wardrobeInside{0.0f};     // floor point inside the wardrobe (Buzz stands here)
	glm::vec3 wardrobeFront{0.0f};      // floor point where Bullseye stands to reach the knobs
	glm::vec3 nightLamp{0.0f};          // bulb of the nightstand lamp (Buzz's room light)
};

// Builds the chest in the middle of the Toy Room and Buzz's furnished bedroom beside the ground-floor
// corridor (star wallpaper, patchwork bed, bookcase, nightstand lamp, poster, rug, football, wardrobe).
StoryRig BuildStoryProps(SceneNode& root, Assets& assets);
