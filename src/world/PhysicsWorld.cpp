#include "PhysicsWorld.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include "Room.h"
#include "scene/SceneNode.h"
#include "scene/Material.h"
#include "math/Ray.h"

namespace {
constexpr float Skin = 0.012f;
glm::vec3 Extent(const glm::mat4& matrix, const glm::vec3& half)
{
	return glm::abs(glm::vec3(matrix[0])) * half.x + glm::abs(glm::vec3(matrix[1])) * half.y
		+ glm::abs(glm::vec3(matrix[2])) * half.z;
}
bool Inside(glm::vec3 p, glm::vec3 lo, glm::vec3 hi)
{
	return p.x > lo.x && p.x < hi.x && p.y > lo.y && p.y < hi.y && p.z > lo.z && p.z < hi.z;
}
bool Sweep(glm::vec3 p, glm::vec3 delta, glm::vec3 lo, glm::vec3 hi, float& time, glm::vec3& normal)
{
	float enter = -1.0f, leave = 1.0f;
	glm::vec3 hitNormal(0.0f);
	for (int axis = 0; axis < 3; ++axis) {
		if (std::abs(delta[axis]) < 1e-7f) {
			if (p[axis] < lo[axis] || p[axis] > hi[axis]) return false;
			continue;
		}
		float a = (lo[axis] - p[axis]) / delta[axis], b = (hi[axis] - p[axis]) / delta[axis];
		glm::vec3 n(0.0f); n[axis] = delta[axis] > 0.0f ? -1.0f : 1.0f;
		if (a > b) std::swap(a, b);
		if (a > enter) { enter = a; hitNormal = n; }
		leave = std::min(leave, b);
		if (enter > leave) return false;
	}
	if (enter < 0.0f || enter > 1.0f) return false;
	time = enter; normal = hitNormal;
	return true;
}
glm::vec3 RoomClamp(glm::vec3 p, const glm::vec3& half)
{
	const glm::vec3 lo(-RoomSize::HalfWidth + half.x + Skin, half.y + Skin, -RoomSize::HalfDepth + half.z + Skin);
	const glm::vec3 hi(RoomSize::HalfWidth - half.x - Skin, RoomSize::Height - half.y - Skin, RoomSize::HalfDepth - half.z - Skin);
	return glm::clamp(p, lo, glm::max(lo, hi));
}
bool Contact(const PhysicsWorld::Bounds& a, const PhysicsWorld::Bounds& b, glm::vec3& n, float& depth)
{
	glm::vec3 overlap = glm::min(a.high, b.high) - glm::max(a.low, b.low);
	if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0) return false;
	int axis = overlap.x < overlap.y ? 0 : 1;
	if (overlap.z < overlap[axis]) axis = 2;
	n = glm::vec3(0.0f);
	n[axis] = (a.low[axis] + a.high[axis]) < (b.low[axis] + b.high[axis]) ? -1.0f : 1.0f;
	depth = overlap[axis];
	return true;
}
}

PhysicsWorld::Bounds PhysicsWorld::ShapeBounds(SceneNode* node)
{
	// Dynamic blocks have an identity parent; scenery uses its scene graph world matrix.
	const glm::mat4 m = node->World();
	const glm::vec3 half = Extent(m, glm::vec3(0.5f));
	return {glm::vec3(m[3]) - half, glm::vec3(m[3]) + half, node};
}

void PhysicsWorld::Init(SceneNode& scene, const std::vector<SceneNode*>& blocks)
{
	scenery.clear(); bodies.clear(); actors.clear(); accumulator = 0.0f;
	for (SceneNode* n : blocks) bodies.push_back({n, n->local});
	scene.ForEach([&](SceneNode& n) { if (n.solid && n.mesh && !IsBlock(&n)) scenery.push_back(&n); });
	RefreshScenery();
}

void PhysicsWorld::RefreshScenery()
{
    sceneryBounds.clear();
    for (SceneNode* n : scenery) {
        bool visible=n->solid;
        for (auto* parent=n; parent && visible; parent=parent->Parent()) visible=parent->visible;
        if (visible) sceneryBounds.push_back(ShapeBounds(n));
    }
}

float PhysicsWorld::FloorHeight(const glm::vec3& feet)
{
    using namespace RoomSize;
    if (feet.x>=StairLeft && feet.x<=StairRight && feet.z<StairTopZ && feet.z>=StairBottomZ) {
        const float cell=(feet.z-StairBottomZ)/(StairTopZ-StairBottomZ)*18.0f;
        return Ground+(-Ground/18.0f)*std::clamp(std::floor(cell)+1.0f,1.0f,18.0f);
    }
    if (feet.z>HouseFront) {
        // Front porch deck and its three steps; the rest of the garden is lawn level.
        const float porch=Ground+0.45f;
        if (feet.x>=8.0f && feet.x<=16.0f && feet.z<=12.5f) return porch;
        if (feet.x>=10.8f && feet.x<=13.2f && feet.z<13.7f) return porch-0.15f*std::floor((feet.z-12.5f)/0.4f);
        return Ground+0.02f;
    }
    if (feet.z<StairBottomZ || feet.y<Slab) return Ground+0.02f;
    return 0.0f;
}

void PhysicsWorld::AddActor(SceneNode* node, glm::vec3 half, glm::vec3 offset)
{
	actors.push_back({node, half, offset});
}
void PhysicsWorld::EnableActor(SceneNode* node, bool enabled)
{
	for (auto& a : actors) if (a.node == node) a.enabled = enabled;
}
void PhysicsWorld::SetActorGrounded(SceneNode* node,bool grounded) { for(auto& actor:actors) if(actor.node==node) actor.grounded=grounded; }
void PhysicsWorld::SetActorShape(SceneNode* node, glm::vec3 half, glm::vec3 offset)
{
	for (auto& a : actors) if (a.node == node) { a.half = half; a.offset = offset; }
}
void PhysicsWorld::SetActorSpine(SceneNode* node, std::vector<float> spine)
{
	for (auto& a : actors) if (a.node == node) a.spine = std::move(spine);
}
void PhysicsWorld::AppendActorBounds(const Actor& a, std::vector<Bounds>& out) const
{
	const glm::mat4 m = a.node->local.Matrix();
	const glm::vec3 h = Extent(m, a.half);
	if (a.spine.empty()) {
		const glm::vec3 c = glm::vec3(m * glm::vec4(a.offset, 1.0f));
		out.push_back({c - h, c + h, a.node});
		return;
	}
	for (float z : a.spine) {
		const glm::vec3 c = glm::vec3(m * glm::vec4(a.offset + glm::vec3(0.0f, 0.0f, z), 1.0f));
		out.push_back({c - h, c + h, a.node});
	}
}
// Moves the whole proxy from `from` to `to` (root positions): every spine segment is swept with the
// same rotation, and the most restrictive result wins. Two passes settle a segment that a later one's
// slide pushed back into contact.
glm::vec3 PhysicsWorld::SlideActor(const Actor& a, const glm::vec3& from, const glm::vec3& to, bool withActors) const
{
	const glm::mat4 m = a.node->local.Matrix();
	const glm::vec3 half = Extent(m, a.half);
	glm::vec3 target = to;
	if (a.spine.empty()) {
		const glm::vec3 offset = glm::vec3(m * glm::vec4(a.offset, 0.0f));
		return Move(from + offset, target + offset, half, a.node, withActors) - offset;
	}
	for (int pass = 0; pass < 2; ++pass)
		for (float z : a.spine) {
			const glm::vec3 offset = glm::vec3(m * glm::vec4(a.offset + glm::vec3(0.0f, 0.0f, z), 0.0f));
			target = Move(from + offset, target + offset, half, a.node, withActors) - offset;
		}
	return target;
}
bool PhysicsWorld::IsBlock(SceneNode* node) const
{
	return std::any_of(bodies.begin(), bodies.end(), [&](const Body& b) { return b.node == node; });
}
std::vector<PhysicsWorld::Bounds> PhysicsWorld::Obstacles(SceneNode* ignore,bool actorContacts) const
{
	std::vector<Bounds> list;
	list.reserve(scenery.size() + actors.size() + bodies.size());
	for (const Bounds& b : sceneryBounds) {
		bool ignored = false;
		if (ignore) for (SceneNode* parent = b.node; parent && !ignored; parent = parent->Parent()) ignored = parent == ignore;
		if (!ignored) list.push_back(b);
	}
	// Broken door boards (bodies that start on the ground floor) are kicked aside by the characters
	// in the contact pass below rather than stopping them on the porch.
	for (const Body& b : bodies) if (b.node != ignore && b.node->visible && b.initial.position.y >= RoomSize::Slab) {
		const glm::vec3 half = Extent(b.node->local.Matrix(), glm::vec3(0.5f));
		list.push_back({b.node->local.position - half, b.node->local.position + half, b.node});
	}
	for (const Actor& a : actors) if (actorContacts && a.enabled && a.node != ignore &&
		!(ignore && ignore!=independentActor && a.node==independentActor)) AppendActorBounds(a, list);
	return list;
}

glm::vec3 PhysicsWorld::Move(const glm::vec3& from, const glm::vec3& to, const glm::vec3& half, SceneNode* ignore, bool withActors) const
{
	// While the story's followers are active nobody is blocked by another character's body, so the
	// group can never jam in a doorway; the house, furniture and blocks still stop everyone.
	const auto obstacles = Obstacles(ignore,withActors && !(scriptedActors && ignore));
 const auto clamp = [&](glm::vec3 p, const glm::vec3& extent) {
  if (houseSpace) {
   using namespace RoomSize;
   // The garden, pavement and street in front of the house (the house walls and fence are solid).
   if (p.z>HouseFront-extent.z-0.2f && exteriorAccess) return glm::clamp(p,glm::vec3(-22+extent.x,Ground+extent.y,HouseFront-extent.z-0.2f),glm::vec3(30-extent.x,Height-extent.y,34-extent.z));
   if (p.x>=StairLeft && p.x<=StairRight && p.z<StairTopZ+extent.z+0.2f && p.z>=StairBottomZ) {
    const float left=StairLeft+extent.x+Skin,right=StairRight-extent.x-Skin;
    // A conservative rotated proxy can briefly exceed the flight width while turning.
    // Centre it instead of passing reversed bounds to std::clamp.
    p.x=left<=right ? std::clamp(p.x,left,right) : 0.5f*(StairLeft+StairRight);
    p.y=std::max(p.y,FloorHeight({p.x,p.y-extent.y,p.z})+extent.y+Skin);
    return p;
   }
   if (from.y<Slab || p.y<Slab || (p.z<StairBottomZ && p.x>HalfWidth)) {
    // The ground floor is a union of boxes: corridor, stair landing, Buzz's bedroom and its doorway.
    // A point outside all of them moves to the nearest one; the solid divider and the door leaf
    // stop movement between the corridor and the bedroom anywhere except through an open door.
    const float bottom=Ground+extent.y+Skin, top=Slab-extent.y-Skin;
    const glm::vec3 boxes[4][2]={
     {{CorridorLeft+extent.x+Skin,bottom,-9.05f+extent.z},{CorridorRight-extent.x-Skin,top,HouseFront-extent.z+0.2f}},
     {{CorridorLeft+extent.x+Skin,bottom,-9.05f+extent.z},{StairRight-extent.x-Skin,top,StairBottomZ}},
     {{BuzzLeft+extent.x+Skin,bottom,-9.05f+extent.z},{CorridorLeft-extent.x-Skin,top,BuzzFront-extent.z-Skin}},
     {{CorridorLeft-extent.x-1.0f,bottom,BuzzDoorLow+extent.z+Skin},{CorridorLeft+extent.x+1.0f,std::min(top,Ground+BuzzDoorHeight-extent.y),BuzzDoorHigh-extent.z-Skin}}};
    glm::vec3 best=p; float nearest=std::numeric_limits<float>::max();
    for (const auto& box:boxes) {
     const glm::vec3 q=glm::clamp(p,box[0],glm::max(box[0],box[1]));
     const float d=glm::distance(q,p);
     if (d<nearest) { nearest=d; best=q; }
    }
    return best;
   }
  }
  // Once an actor/camera is in the connected hallway, keep it in that corridor even
  // when a requested step crosses a side boundary. Falling back to RoomClamp here
  // teleports it through the connected doorway into the room at the corridor corner.
  if (hallway && p.x > RoomSize::HalfWidth - extent.x - Skin
   && ((p.z >= RoomSize::DoorLow + extent.z + 0.1f && p.z <= RoomSize::DoorHigh - extent.z - 0.1f)
    || (from.x > RoomSize::HalfWidth && from.z >= RoomSize::DoorLow && from.z <= RoomSize::DoorHigh))
   && p.y + extent.y < RoomSize::DoorHeight - 0.1f) {
   return glm::clamp(p, glm::vec3(-RoomSize::HalfWidth+extent.x+Skin,extent.y+Skin,RoomSize::DoorLow+extent.z+0.1f),
    glm::vec3(RoomSize::HallEnd-extent.x-0.1f,RoomSize::DoorHeight-extent.y-0.1f,RoomSize::DoorHigh-extent.z-0.1f));
  }
  return RoomClamp(p,extent);
 };
	glm::vec3 p = clamp(from, half), delta = clamp(to, half) - p;
	// Recover safely after edits, teleporting presets or a moved block overlapping the camera.
	for (int pass = 0; pass < 12; ++pass) {
		bool overlap = false;
		for (const Bounds& b : obstacles) {
			const glm::vec3 lo = b.low - half - Skin, hi = b.high + half + Skin;
			if (!Inside(p, lo, hi)) continue;
			glm::vec3 correction(0.0f); float best = std::numeric_limits<float>::max();
			for (int axis = 0; axis < 3; ++axis) {
				const float left = lo[axis] - p[axis], right = hi[axis] - p[axis];
				for (float push : {left, right}) {
					glm::vec3 candidate = p; candidate[axis] += push;
					candidate = clamp(candidate, half);
					bool clear = false;
					// Follow this exit through adjacent bounds, rather than oscillating between them.
					for (size_t chain = 0; chain <= obstacles.size(); ++chain) {
						clear = true;
						for (const Bounds& neighbor : obstacles) {
							const glm::vec3 nlo = neighbor.low - half - Skin, nhi = neighbor.high + half + Skin;
							if (!Inside(candidate, nlo, nhi)) continue;
							clear = false;
							candidate[axis] = push < 0 ? nlo[axis] : nhi[axis];
							candidate = clamp(candidate, half);
							break;
						}
						if (clear) break;
					}
					if (!clear) continue;
					const float distance = glm::distance(candidate, p);
					if (distance < best) { best = distance; correction = candidate - p; }
				}
			}
			p = clamp(p + correction, half); overlap = true;
		}
		if (!overlap) break;
	}
	for (int pass = 0; pass < 5 && glm::dot(delta, delta) > 1e-10f; ++pass) {
		float best = 1.0f; glm::vec3 normal(0.0f);
		for (const Bounds& b : obstacles) {
			float t; glm::vec3 n;
			if (Sweep(p, delta, b.low - half - Skin, b.high + half + Skin, t, n) && t < best) { best = t; normal = n; }
		}
		p += delta * std::max(0.0f, best - 0.0001f);
		if (best >= 1.0f) break;
		delta *= (1.0f - best);
		delta -= normal * std::min(0.0f, glm::dot(delta, normal)); // slide along the contacted face
	}
	return clamp(p, half);
}

void PhysicsWorld::ConstrainActor(SceneNode* node, const glm::vec3& previous)
{
	for (const Actor& a : actors) if (a.node == node && a.enabled) {
		const glm::vec3 available(RoomSize::HalfWidth - 0.6f, RoomSize::Height * 0.5f - 0.25f, RoomSize::HalfDepth - 0.6f);
		const glm::vec3 current = Extent(node->local.Matrix(), a.half);
		const float fit = std::min({1.0f, available.x / std::max(current.x, 0.001f), available.y / std::max(current.y, 0.001f), available.z / std::max(current.z, 0.001f)});
		node->local.scale *= fit;
        if (houseSpace) {
            const float floor=FloorHeight({node->local.position.x,previous.y,node->local.position.z});
            const bool stairs=node->local.position.x>=RoomSize::StairLeft && node->local.position.z<RoomSize::StairTopZ;
            if ((a.grounded && (stairs || previous.y<RoomSize::Slab)) || node->local.position.y<floor) node->local.position.y=floor;
        }
        node->local.position = SlideActor(a, previous, node->local.position, true);
		return;
	}
}
void PhysicsWorld::SeparateActors(const std::vector<SceneNode*>& byPriority, SceneNode* anchor)
{
	std::vector<const Actor*> members; // same order as byPriority: members[i] has right of way over members[j > i]
	for (SceneNode* node : byPriority)
		for (const Actor& a : actors) if (a.node == node && a.enabled) members.push_back(&a);
	// A few relaxation sweeps: separating one pair can press a body into a third one.
	for (int sweep = 0; sweep < 3; ++sweep) {
		bool moved = false;
		for (size_t i = 0; i < members.size(); ++i)
			for (size_t j = i + 1; j < members.size(); ++j) {
				const Actor& a = *members[i];
				const Actor& b = *members[j];
				if (a.node == b.node) continue;
				std::vector<Bounds> ba, bb;
				AppendActorBounds(a, ba); AppendActorBounds(b, bb);
				// Deepest overlap among the segment pairs, measured on the ground plane (characters stand
				// side by side; pushing one up or down would lift it off the floor).
				// Two ways apart: along x or along z, each by that axis' overlap (for the deepest segment pair).
				glm::vec3 pushX(0.0f), pushZ(0.0f); float deepest = 0.0f;
				for (const Bounds& p : ba) for (const Bounds& q : bb) {
					if (p.high.y <= q.low.y || q.high.y <= p.low.y) continue; // one passes above the other
					const float ox = std::min(p.high.x, q.high.x) - std::max(p.low.x, q.low.x);
					const float oz = std::min(p.high.z, q.high.z) - std::max(p.low.z, q.low.z);
					if (ox <= 0.0f || oz <= 0.0f) continue;
					const float depth = std::min(ox, oz);
					if (depth <= deepest) continue;
					deepest = depth;
					const glm::vec3 d = (q.low + q.high - p.low - p.high) * 0.5f; // centre of p -> centre of q
					pushX = {(d.x >= 0.0f ? 1.0f : -1.0f) * (ox + Skin), 0.0f, 0.0f};
					pushZ = {0.0f, 0.0f, (d.z >= 0.0f ? 1.0f : -1.0f) * (oz + Skin)};
				}
				if (deepest <= 0.0f) continue;
				// The lower-priority body (b) yields first; its push slides against the house and furniture
				// exactly like a step of walking. The shorter push is tried first; if a wall pins b on that axis
				// (a toy standing beside a doorway), it steps aside along the other axis instead, so the higher
				// body can pass. Whatever b still cannot give, the higher one (a) gives back.
				glm::vec3& pb = b.node->local.position;
				const glm::vec3 before = pb;
				glm::vec3 push = glm::length(pushX) < glm::length(pushZ) ? pushX : pushZ;
				if (b.node != anchor) {
					pb = SlideActor(b, before, before + push, false);
					if (glm::dot(pb - before, glm::normalize(push)) < 0.5f * glm::length(push)) {
						const glm::vec3 other = push == pushX ? pushZ : pushX;
						const glm::vec3 alternative = SlideActor(b, before, before + other, false);
						if (glm::dot(alternative - before, glm::normalize(other)) > glm::dot(pb - before, glm::normalize(push))) {
							pb = alternative; push = other;
						}
					}
				}
				const glm::vec3 axis = glm::normalize(push);
				const float remaining = glm::length(push) - glm::dot(pb - before, axis);
				if (remaining > 1e-4f && a.node != anchor) {
					glm::vec3& pa = a.node->local.position;
					pa = SlideActor(a, pa, pa - axis * remaining, false);
				}
				moved = true;
			}
		if (!moved) break;
	}
}

glm::vec3 PhysicsWorld::MoveCamera(const glm::vec3& previous, const glm::vec3& desired) const
{
	// The camera passes through characters (they never hide the view); only the house stops it.
	return Move(previous, desired, glm::vec3(0.20f), nullptr, false);
}
glm::vec3 PhysicsWorld::CameraSightline(const glm::vec3& target, const glm::vec3& desired, SceneNode* ignored) const
{
	glm::vec3 delta = desired - target; float best = 1.0f;
	for (const Bounds& b : Obstacles(ignored, false)) {
		float t; glm::vec3 n;
		if (Sweep(target, delta, b.low - 0.22f, b.high + 0.22f, t, n)) best = std::min(best, t);
	}
	return target + delta * std::max(0.0f, best - 0.005f);
}
void PhysicsWorld::ResetBlocks()
{
	lastLaserHit = nullptr;
	for (Body& b : bodies) { b.node->local = b.initial; b.velocity = b.angular = glm::vec3(0.0f); }
}
void PhysicsWorld::PushBlock(SceneNode* node, const glm::vec3& impulse,const glm::vec3& angular)
{
	for (Body& b : bodies) if (b.node == node) { b.velocity += impulse; b.angular+=angular; }
}
void PhysicsWorld::StopBlock(SceneNode* node)
{
	for (Body& b : bodies) if (b.node == node) b.velocity = b.angular = glm::vec3(0.0f);
}

void PhysicsWorld::Update(float dt)
{
	laserCooldown = std::max(0.0f, laserCooldown - dt);
	constexpr float step = 1.0f / 120.0f;
	// At most 6 steps per frame: a slow frame must not schedule even more work for the next one.
	accumulator = std::min(accumulator + std::min(dt, 0.1f), 6.0f * step);
	// Furniture and actors stand still while the blocks are simulated, so their boxes are computed
	// once per frame instead of once per block, per contact iteration and per step.
	RefreshScenery();
	std::vector<Bounds> actorBounds;
	for (const Actor& actor : actors) if (actor.enabled) AppendActorBounds(actor, actorBounds);
	// A block's rotation is fixed during the contact pass; only its position is corrected.
	std::vector<glm::vec3> halves(bodies.size());
	auto bounds = [&](size_t i) {
		const glm::vec3& p = bodies[i].node->local.position;
		return Bounds{p - halves[i], p + halves[i], bodies[i].node};
	};
	auto overlaps = [](const Bounds& a, const Bounds& b) {
		return glm::all(glm::lessThan(a.low, b.high)) && glm::all(glm::lessThan(b.low, a.high));
	};
	while (accumulator >= step) {
		accumulator -= step;
		for (size_t i = 0; i < bodies.size(); ++i) {
			Body& b = bodies[i];
			if (!b.node->visible) continue;
			const glm::vec3 available(RoomSize::HalfWidth - 0.6f, RoomSize::Height * 0.5f - 0.25f, RoomSize::HalfDepth - 0.6f);
			const glm::vec3 current = Extent(b.node->local.Matrix(), glm::vec3(0.5f));
			const float fit = std::min({1.0f, available.x / std::max(current.x, 0.001f), available.y / std::max(current.y, 0.001f), available.z / std::max(current.z, 0.001f)});
			b.node->local.scale *= fit;
			b.velocity.y -= 9.81f * step;
			b.node->local.position += b.velocity * step;
			b.node->local.rotation += b.angular * step;
			b.angular *= std::exp(-step * 1.7f);
			halves[i] = Extent(b.node->local.Matrix(), glm::vec3(0.5f));
			Bounds box = bounds(i);
			const float floor=houseSpace && b.initial.position.y<RoomSize::Slab ? FloorHeight({b.node->local.position.x,RoomSize::Ground,b.node->local.position.z})-0.02f : 0.0f;
			if (box.low.y < floor) {
				b.node->local.position.y += floor-box.low.y;
				if (b.velocity.y < 0) b.velocity.y = -b.velocity.y * 0.14f;
				b.velocity.x *= std::exp(-step * 5.0f); b.velocity.z *= std::exp(-step * 5.0f);
				b.angular *= std::exp(-step * 7.0f);
				if (glm::length(b.velocity) < 0.18f && glm::length(b.angular) < 8.0f) {
					for (int axis : {0, 2}) b.node->local.rotation[axis] += (std::round(b.node->local.rotation[axis] / 90.0f) * 90.0f - b.node->local.rotation[axis]) * step * 5.0f;
					halves[i] = Extent(b.node->local.Matrix(), glm::vec3(0.5f));
				}
			}
			const glm::vec3& half = halves[i];
			const glm::vec3 clamped = floor<0 ? glm::clamp(b.node->local.position,
                glm::vec3(-12.0f+half.x,RoomSize::Ground+half.y,6.0f+half.z),glm::vec3(20.0f-half.x,RoomSize::Slab-half.y,26.0f-half.z)) : RoomClamp(b.node->local.position,half);
			for (int axis : {0, 2}) if (clamped[axis] != b.node->local.position[axis]) b.velocity[axis] *= -0.25f;
			b.node->local.position.x = clamped.x; b.node->local.position.z = clamped.z;
			if (b.node->local.position.y > clamped.y && b.velocity.y > 0) b.velocity.y *= -0.15f;
			b.node->local.position.y = std::min(b.node->local.position.y, clamped.y + 0.001f);
		}
		// Broad phase: only furniture near a block can touch it during this step's contact pass.
		nearby.resize(bodies.size());
		for (size_t i = 0; i < bodies.size(); ++i) {
			nearby[i].clear();
			if (!bodies[i].node->visible) continue;
			Bounds reach = bounds(i);
			reach.low -= 0.25f; reach.high += 0.25f;
			for (const Bounds& wall : sceneryBounds) if (overlaps(reach, wall)) nearby[i].push_back(&wall);
		}
		// Iterative separation keeps stacks and furniture solid after an impact.
		for (int iteration = 0; iteration < 6; ++iteration) {
			for (size_t i = 0; i < bodies.size(); ++i) {
				Body& a = bodies[i]; if (!a.node->visible) continue; glm::vec3 n; float depth;
				for (const Bounds* wall : nearby[i]) if (Contact(bounds(i), *wall, n, depth)) {
					a.node->local.position += n * (depth + 0.001f);
					const float speed = glm::dot(a.velocity, n);
					if (speed < 0) a.velocity -= n * speed * 1.15f;
				}
				for (const Bounds& actor : actorBounds) if (Contact(bounds(i), actor, n, depth)) {
					a.node->local.position += n * (depth + 0.001f);
					const float speed = glm::dot(a.velocity, n);
					if (speed < 0) a.velocity -= n * speed;
				}
				for (size_t j = i + 1; j < bodies.size(); ++j) {
					Body& b = bodies[j]; if (!b.node->visible) continue;
					if (!Contact(bounds(i), bounds(j), n, depth)) continue;
					a.node->local.position += n * (depth * 0.5f + 0.0001f);
					b.node->local.position -= n * (depth * 0.5f + 0.0001f);
					const float closing = glm::dot(a.velocity - b.velocity, n);
					if (closing < 0) { const glm::vec3 impulse = n * (-closing * 0.54f); a.velocity += impulse; b.velocity -= impulse; }
					if (std::abs(n.y) > 0.5f) { a.velocity.x *= 0.98f; a.velocity.z *= 0.98f; b.velocity.x *= 0.98f; b.velocity.z *= 0.98f; }
				}
			}
		}
		for (size_t i = 0; i < bodies.size(); ++i) {
			if (!bodies[i].node->visible) continue;
			const Bounds box = bounds(i);
			const float floor=houseSpace && bodies[i].initial.position.y<RoomSize::Slab ? FloorHeight({bodies[i].node->local.position.x,RoomSize::Ground,bodies[i].node->local.position.z})-0.02f : 0.0f;
			if (box.low.y < floor) bodies[i].node->local.position.y += floor-box.low.y;
		}
	}
}

float PhysicsWorld::FireLaser(const glm::vec3& origin, const glm::vec3& direction, float /*dt*/, SceneNode* shooter)
{
	float closest = 24.0f; Body* hitBody = nullptr; lastLaserHit=nullptr;
	Ray ray{origin, direction};
	for (const Bounds& b : Obstacles(shooter)) {
		float distance = -1.0f;
		if (b.node->mesh) {
			// Blocks update their own pose between scene graph updates.
			const glm::mat4 m = IsBlock(b.node) ? b.node->local.Matrix() : b.node->World();
			distance = RayIntersect::Object(b.node->mesh->Type(), glm::inverse(m), ray).t;
		} else {
			// Movement proxies include limb clearance; the laser only hits visible geometry.
			b.node->ForEach([&](SceneNode& shape) {
				if (!shape.visible || !shape.mesh || !shape.material || shape.material->opacity < 0.9f) return;
				const float t = RayIntersect::Object(shape.mesh->Type(), glm::inverse(shape.World()), ray).t;
				if (t > 0 && (distance < 0 || t < distance)) distance = t;
			});
		}
		if (distance < 0 || distance >= closest) continue;
		closest = distance; hitBody = nullptr; lastLaserHit=b.node;
		for (Body& body : bodies) if (body.node == b.node) hitBody = &body;
	}
	// Room faces stop the beam; the connected doorway admits it into the hallway.
	const glm::vec3 roomLow(-RoomSize::HalfWidth, 0, -RoomSize::HalfDepth), roomHigh(RoomSize::HalfWidth, RoomSize::Height, RoomSize::HalfDepth);
	for (int axis = 0; axis < 3; ++axis) if ((!houseSpace || (origin.y>=0 && origin.x<RoomSize::HalfWidth)) && std::abs(direction[axis]) > 1e-6f) {
		const float wall = ((direction[axis] > 0 ? roomHigh[axis] : roomLow[axis]) - origin[axis]) / direction[axis];
		const glm::vec3 contact = origin + direction * wall;
		const bool doorway = hallway && axis==0 && direction.x>0 && contact.z>RoomSize::DoorLow
			&& contact.z<RoomSize::DoorHigh && contact.y>0 && contact.y<RoomSize::DoorHeight;
		if (wall > 0 && wall < closest && !doorway) { closest = wall; hitBody = nullptr; lastLaserHit=nullptr; }
	}
	if (hitBody && laserCooldown <= 0.0f) {
		const glm::vec3 contact = origin + direction * closest;
		hitBody->velocity += direction * 3.5f + glm::vec3(0, 1.8f, 0);
		hitBody->angular += glm::cross(contact - hitBody->node->local.position, direction * 3.5f) * 170.0f + glm::vec3(32, 15, 22);
		laserCooldown = 0.18f;
		lastLaserHit = hitBody->node;
	}
	return closest;
}
