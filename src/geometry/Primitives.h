#pragma once

#include "Mesh.h"

// Builders for the five basic shapes. Every object in the scene is assembled from these.
//
// All shapes are UNIT sized and centred on the origin, so a model matrix scale of (w, h, d)
// gives an object of exactly w x h x d units:
//
//   Plane    : 1 x 1 square in the XZ plane (y = 0), facing +Y
//   Cube     : [-0.5, 0.5] on every axis
//   Sphere   : radius 0.5
//   Cylinder : radius 0.5, y from -0.5 to +0.5 (axis = Y), with caps
//   Cone     : base radius 0.5 at y = -0.5, apex at y = +0.5, with base cap
//
// Triangles are wound COUNTER-CLOCKWISE when seen from outside (OpenGL's default front face),
// which lets back-face culling discard the hidden inside faces.
namespace Primitives {

MeshData Plane();
MeshData Cube();
MeshData Sphere(int stacks = 18, int sectors = 32);
MeshData Cylinder(int sectors = 32);
MeshData Cone(int sectors = 32);

}
