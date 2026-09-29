#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "MatrixStack.h"
#include "Transform.h"

class Mesh;
struct Material;

// One node of the scene graph (hierarchical model).
//
// A node has a LOCAL transform relative to its parent. Its WORLD matrix is
//   world = parent.world * local
// computed for the whole tree in one depth-first pass with a MatrixStack.
//
// Nodes that carry a mesh are "shapes"; nodes without a mesh are "joints" (shoulder, hip, saddle...)
// that only position and rotate their children. Non-uniform scale is put on shape nodes only, so it
// never distorts the children.
class SceneNode {
public:
	explicit SceneNode(std::string name) : name(std::move(name)) {}

	std::string name;
	Transform local;
	const Mesh* mesh = nullptr;
	const Material* material = nullptr;
	bool visible = true;
	bool solid = false; // physical furniture; thin decoration and light effects stay non-solid
	int ownerId = -1; // id of the selectable object this node belongs to (-1 = static scenery)

	SceneNode* Parent() const { return parent; }
	const std::vector<std::unique_ptr<SceneNode>>& Children() const { return children; }
	const glm::mat4& World() const { return world; }
	glm::vec3 WorldPosition() const { return glm::vec3(world[3]); }

	// Creates an empty child (joint).
	SceneNode* AddChild(const std::string& childName);
	// Creates a child that draws `mesh` with `material`, placed at `position`, sized by `size`,
	// rotated by `rotationDeg` (Euler degrees, see Transform).
	SceneNode* AddShape(const std::string& childName, const Mesh* shapeMesh, const Material* shapeMaterial,
		const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotationDeg = glm::vec3(0.0f));

	// Re-parenting (used for mounting). The caller is responsible for adjusting `local`.
	void AttachChild(std::unique_ptr<SceneNode> child);
	std::unique_ptr<SceneNode> DetachChild(SceneNode* child);

	// Recomputes world matrices of this node and all descendants.
	void UpdateWorld(MatrixStack& stack);
	void UpdateWorld(const glm::mat4& parentWorld);

	SceneNode* Find(const std::string& nodeName);
	void ForEach(const std::function<void(SceneNode&)>& fn);
	void SetOwner(int id); // tags the whole subtree

private:
	SceneNode* parent = nullptr;
	std::vector<std::unique_ptr<SceneNode>> children;
	glm::mat4 world{ 1.0f };
};
