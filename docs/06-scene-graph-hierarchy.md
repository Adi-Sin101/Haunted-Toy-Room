# 06 — Scene Graph and Hierarchical Transformations

Files: `src/scene/SceneNode.*`, `src/scene/MatrixStack.h`, `ToyRoomApp::Mount / Dismount`.

## 1. Why a hierarchy

A character's forearm should follow the upper arm, which follows the shoulder, which follows the torso,
which follows the character. Writing every part's **world** position by hand would mean recomputing all of
them whenever anything moves. Instead each part stores only its **local** transform relative to its parent:

```
world(node) = world(parent) · local(node)
            = local(root) · local(child) · local(grandchild) · …
```

Change one matrix (e.g. the shoulder's rotation) and every descendant moves correctly.

## 2. `SceneNode`

```cpp
class SceneNode {
    std::string name;
    Transform   local;        // relative to the parent (see 04)
    const Mesh* mesh;         // null for pure joints
    const Material* material;
    bool        visible;
    int         ownerId;      // which selectable object this node belongs to
    SceneNode*  parent;
    std::vector<std::unique_ptr<SceneNode>> children;   // parent OWNS its children
    glm::mat4   world;        // cached result of the last UpdateWorld pass
};
```

Two kinds of nodes:

* **Joints** (no mesh) — a pivot: shoulder, hip, neck, saddle seat, wheel steering, lamp arm. Rotating a
  joint rotates everything under it *about the joint's origin*.
* **Shapes** (mesh + material) — the visible pieces. Their local scale turns the unit primitive into the
  right size. Non-uniform scale is put **only on shapes (leaves)**, never on joints, so it never distorts
  the children.

`AddShape(name, mesh, material, position, size, rotation)` creates a shape child in one line.

## 3. `MatrixStack` — computing world matrices

```cpp
void SceneNode::UpdateWorld(MatrixStack& stack) {
    stack.push();                     // remember the parent's matrix
    stack.multiply(local.Matrix());   // current = parent · local
    world = stack.top();
    for (auto& child : children)
        child->UpdateWorld(stack);    // children start from OUR matrix
    stack.pop();                      // restore the parent's matrix for our siblings
}
```

This is the classic push / multiply / draw / pop pattern of hierarchical modelling. One depth-first pass
per frame (`scene->UpdateWorld(identity)`) updates the whole scene. The renderer then just reads
`node.World()`.

## 4. Example: Woody's right arm

```
Woody (root)          T(x, 0, z) · Ry(heading)                    ← driving moves only this node
 └─ Pelvis            T(0, 0.85 + bob, 0)
     └─ Torso         T(0, 0.09, 0)
         └─ RightShoulder  T(−0.27, 0.46, 0) · Rx(swing) · Rz(−6°)  ← walk cycle rotates only this node
             ├─ UpperArm   T(0, −0.25, 0) · S(0.11, 0.5, 0.11)   cylinder
             └─ Hand       T(0, −0.55, 0) · S(0.13)               sphere
```

World matrix of the hand:

```
M_hand = M_root · M_pelvis · M_torso · M_shoulder · M_hand_local
```

Because the arm hangs along the joint's −Y axis and the joint is at the shoulder, `Rx(swing)` swings the
arm like a pendulum about the shoulder — no "rotate about a fixed point" matrix needed.

## 5. Dynamic re-parenting: Jessie mounts Bullseye (R)

Requirement: Jessie must follow Bullseye through the **hierarchy**, not by copying his position each frame,
and must never jump/teleport when mounting or dismounting.

### Mount (`ToyRoomApp::Mount`)

```
1. distance check:  |Jessie.xz − Bullseye.xz| < 2.2   (otherwise print "Jessie is too far…")
2. W  = Jessie.world                                   (where she is right now)
3. detach Jessie from the world root, attach under Bullseye's Seat joint
4. local = inverse(Seat.world) · W                    (same place, expressed in seat coordinates)
       position = local[3],  yaw = atan2(local[2].x, local[2].z)
5. glide (0.7 s, smoothstep + small hop) to local = T(0, −0.85, 0), yaw 0
       → her hips (0.85 above her feet) sit exactly on the seat, facing forward
6. switch her pose to "seated" (legs astride, hands forward on the reins)
```

Step 4 is the key equation: since `world = parent.world · local`, choosing
`local = parent.world⁻¹ · world` keeps the world placement identical at the moment of re-parenting.

The hierarchy is now:

```
Bullseye
└─ Body
   ├─ Barrel, Belly …
   ├─ Saddle
   │  └─ Seat
   │     └─ Jessie          ← child of the horse
   │        ├─ Pelvis …
   ├─ Neck ─ Head
   ├─ Tail
   └─ 4 legs
```

From now on Bullseye's movement, turning, even his gallop bob (on the Body joint) automatically carry
Jessie: her world matrix is `M_bullseye · M_body · M_saddle · M_seat · M_jessie`.

While mounted, pressing **2** (Jessie) or **3** (Bullseye) both drive the horse; `DrivenCharacter()` returns
Bullseye for the mounted unit.

![Jessie riding Bullseye](images/mounted.png)

### Dismount (`ToyRoomApp::Dismount`)

```
1. W = Jessie.world                      (computed through the whole chain)
2. detach from Seat, attach to the world root
3. local = W   (the root's parent is the identity)   → no jump
4. glide (0.6 s) to a point 1.1 units on Bullseye's left side, on the floor, facing his heading
5. pose back to standing; W/S/A/D control Jessie again
```

Left side vector for a heading h (forward = (sin h, 0, cos h)): `left = (cos h, 0, −sin h)`.

## 6. Other hierarchies in the scene

| Object | Joints that animate | Effect |
|---|---|---|
| Humanoids | Pelvis (bob), LeftHip/RightHip, LeftShoulder/RightShoulder, Head | walk cycle, idle look-around |
| Buzz | + Wings (scale X), RightShoulder raised for the laser | wings unfold when airborne |
| Bullseye | Body (bob), Neck, Head, Tail, 4 hips | gallop, tail swish |
| RC car | 4 wheel joints (steer Y) → Spin joints (roll X) | steering + rolling wheels |
| Desk lamp | Lamp (swivel Y) → ArmJoint (tilt X) → HeadJoint (tilt X) → LightAnchor | the spotlight follows the head |
| Ball | Ball (position) → BallShape (basis = accumulated rolling rotation) | rolls without slipping |
| Ghost | Ghost (path) → Body (sway Z) | floats and wobbles |
| Penny the cat | Body (pitch on stairs / sitting, roll when asleep) → Head (look-at yaw), Tail → TailTip, 4 legs | trot, sit, sleep ([18](18-house-and-penny.md)) |
| House | FrontDoor, RoomDoorLeft/Right, StairDoor hinge joints (yaw); Gable joints (non-uniform scale) | doors open as Penny arrives |
| HouseExterior / HouseInterior | group nodes; `visible = false` hides a whole subtree | the outside and the ground floor are hidden once Penny is upstairs |

**Hidden subtrees cost nothing.** `SceneNode::UpdateWorld` stops at a node whose `visible` is false (its
descendants keep their last world matrices and are recomputed when it becomes visible again), and
`Renderer::Collect` does not descend into it either. Hiding `HouseExterior` removes about 200 shapes from
both passes in one assignment.

Lights are attached the same way: every frame the lamp's spot light takes its position from the
`LightAnchor` node's world matrix and its direction from the head joint's −Y axis, so moving or tilting the
lamp (manually or in edit mode) moves the light cone.

## 7. Selection ownership

`AddSelectable` tags a whole subtree with an `ownerId` (`SceneNode::SetOwner`). The renderer highlights all
draw items of the selected owner and picking maps a hit part back to its owner. (The ray tracer no longer
groups parts by owner: its bounding volume hierarchy is built over all parts, [12 §4.1](12-ray-tracing.md).)
When Jessie is re-parented her nodes keep her owner id, so she is
still selected/highlighted as Jessie while riding.
