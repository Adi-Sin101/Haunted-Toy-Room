#pragma once

#include <glm/glm.hpp>

class Assets;
class SceneNode;

// Handles to the parts of the house that move or are switched on and off.
struct HouseRig {
	SceneNode* exterior = nullptr;      // facade, roof, porch, garage, garden, fence, trees: hidden once indoors
	SceneNode* interior = nullptr;      // ground-floor corridor, landing, stairs: hidden once the stair door shuts
	SceneNode* stairDoor = nullptr;     // door in the hallway's z = DoorLow wall at the top of the stairs,
	                                    //   hinged at x = StairLeft, rotation.y 0 (closed) .. 90 (open over the stairs)
	SceneNode* frontDoor = nullptr;     // hinge joint, rotation.y 0 (closed) .. 90 (open inward)
	SceneNode* roomDoorLeft = nullptr;  // toy-room double door, hinges at z = DoorLow / DoorHigh
	SceneNode* roomDoorRight = nullptr; //   open into the hallway: left 0..90, right 0..-90 degrees
	SceneNode* outdoorSun = nullptr;    // the sun as seen from the garden (in front of the sky backdrop)
};

// Builds the house around the toy room (which is its upper floor):
//   * interior: ground-floor corridor from the front door, stair landing, an 18-step flight that
//     climbs into the hallway, the stairwell walls and ceiling, the stair-head door and the toy room's
//     double door;
//   * exterior: lap-sided walls, a gabled shingle roof with chimney, a front porch with brick-based
//     columns, railing and steps, decorative windows with a flower box, a garage, lawn, path,
//     pavement, a white picket fence with a gate, trees, shrubs and a mailbox.
// Everything is made of the five primitives; walls are thin cubes so they can be seen from outside.
HouseRig BuildHouse(SceneNode& root, Assets& assets);
