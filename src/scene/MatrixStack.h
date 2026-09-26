#pragma once

#include <vector>

#include <glm/glm.hpp>

// Classic matrix stack for hierarchical modelling.
//
//   push()        -> save the current matrix (enter a child)
//   multiply(M)   -> current = current * M  (apply the child's local transform)
//   pop()         -> restore the parent's matrix (leave the child)
//
// Because the parent's matrix is multiplied on the LEFT, a child is always expressed in its
// parent's coordinate system: moving/rotating the parent moves every descendant with it.
class MatrixStack {
public:
	MatrixStack() { stack.reserve(32); stack.emplace_back(1.0f); }

	void push() { stack.push_back(stack.back()); }
	void pop() { if (stack.size() > 1) stack.pop_back(); }
	void multiply(const glm::mat4& m) { stack.back() = stack.back() * m; }
	void loadIdentity() { stack.back() = glm::mat4(1.0f); }
	const glm::mat4& top() const { return stack.back(); }
	size_t depth() const { return stack.size(); }

private:
	std::vector<glm::mat4> stack;
};
