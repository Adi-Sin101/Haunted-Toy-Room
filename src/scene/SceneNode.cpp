#include "SceneNode.h"

#include <algorithm>

SceneNode* SceneNode::AddChild(const std::string& childName)
{
	auto node = std::make_unique<SceneNode>(childName);
	node->parent = this;
	node->ownerId = ownerId;
	children.push_back(std::move(node));
	return children.back().get();
}

SceneNode* SceneNode::AddShape(const std::string& childName, const Mesh* shapeMesh, const Material* shapeMaterial,
	const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotationDeg)
{
	SceneNode* node = AddChild(childName);
	node->mesh = shapeMesh;
	node->material = shapeMaterial;
	node->local.position = position;
	node->local.scale = size;
	node->local.rotation = rotationDeg;
	return node;
}

void SceneNode::AttachChild(std::unique_ptr<SceneNode> child)
{
	child->parent = this;
	children.push_back(std::move(child));
}

std::unique_ptr<SceneNode> SceneNode::DetachChild(SceneNode* child)
{
	auto it = std::find_if(children.begin(), children.end(),
		[child](const std::unique_ptr<SceneNode>& c) { return c.get() == child; });
	if (it == children.end())
		return nullptr;
	std::unique_ptr<SceneNode> detached = std::move(*it);
	children.erase(it);
	detached->parent = nullptr;
	return detached;
}

void SceneNode::UpdateWorld(MatrixStack& stack)
{
	stack.push();
	stack.multiply(local.Matrix());
	world = stack.top();
	for (auto& child : children)
		child->UpdateWorld(stack);
	stack.pop();
}

void SceneNode::UpdateWorld(const glm::mat4& parentWorld)
{
	MatrixStack stack;
	stack.multiply(parentWorld);
	UpdateWorld(stack);
}

SceneNode* SceneNode::Find(const std::string& nodeName)
{
	if (name == nodeName)
		return this;
	for (auto& child : children)
		if (SceneNode* found = child->Find(nodeName))
			return found;
	return nullptr;
}

void SceneNode::ForEach(const std::function<void(SceneNode&)>& fn)
{
	fn(*this);
	for (auto& child : children)
		child->ForEach(fn);
}

void SceneNode::SetOwner(int id)
{
	ForEach([id](SceneNode& n) { n.ownerId = id; });
}
