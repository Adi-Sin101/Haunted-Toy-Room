# 02 — OpenGL Pipeline Fundamentals

Everything visible in the Haunted Toy Room is made of triangles that we define ourselves.
This chapter explains how a list of numbers in C++ becomes pixels on the screen, and which class in
the project is responsible for each step.

## 1. The programmable pipeline (OpenGL 3.3 Core)

```
 C++ (CPU)                                GPU
 ─────────                                ───────────────────────────────────────────────────────
 vertices[] ─► VBO ─┐
 indices[]  ─► EBO ─┼─► VAO ─► glDrawElements ─► VERTEX SHADER ─► primitive assembly ─► clipping
 layout     ───────┘                              (per vertex)      (3 indices = 1 tri)   + divide by w
                                                                                              │
          framebuffer ◄─ depth test / blending ◄─ FRAGMENT SHADER ◄─ rasterisation ◄─ viewport
                                                   (per pixel)       (interpolates        transform
                                                                      vertex outputs)
```

| Stage | What happens | Where in our code |
|---|---|---|
| Vertex specification | Vertex data is copied to GPU buffers and its layout is described | `Mesh` constructor (`src/geometry/Mesh.cpp`) using `VAO`, `VBO`, `EBO` |
| Vertex shader | Runs once per vertex: `gl_Position = proj * view * model * vec4(pos, 1)` | `shaders/lit.vert`, `shaders/gouraud.vert` |
| Primitive assembly | Every 3 indices from the EBO form one triangle | `glDrawElements(GL_TRIANGLES, …)` in `Mesh::Draw` |
| Clipping + perspective divide | Triangles outside the view volume are cut; `xyz / w` gives normalized device coordinates (NDC, −1…1) | fixed function |
| Viewport transform | NDC → window pixels | `glViewport` in `Application` |
| Face culling | Triangles whose screen-space winding is clockwise are back faces and are skipped | `glEnable(GL_CULL_FACE)` in `Renderer::Render` |
| Rasterisation | Finds the pixels covered by each triangle and interpolates the vertex outputs (normal, uv, world position) | fixed function |
| Fragment shader | Runs per pixel: lighting, texturing | `shaders/lit.frag`, `shaders/gouraud.frag` |
| Depth test | Keeps the fragment nearest to the camera (`GL_LESS` against the depth buffer) | `glEnable(GL_DEPTH_TEST)` |
| Blending | Mixes transparent fragments with what is already drawn | transparent pass in `Renderer::Render` |

## 2. Buffers: VBO, EBO, VAO

These three classes come from the original lab template. They were kept (same names, same
`Bind / Unbind / Delete` interface) and made safe: each object now owns its OpenGL handle, cannot be
copied, and frees the GPU memory in its destructor.

### VBO — Vertex Buffer Object (`src/gl/VBO.*`)
A block of GPU memory that holds the vertex data.

```cpp
glGenBuffers(1, &ID);                                   // create a buffer name
glBindBuffer(GL_ARRAY_BUFFER, ID);                      // make it the current array buffer
glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); // allocate + copy
```
`GL_STATIC_DRAW` tells the driver the data is written once and drawn many times (all our meshes).
`DebugLines` uses `GL_DYNAMIC_DRAW` because it is refilled every frame.

### EBO — Element Buffer Object (`src/gl/EBO.*`)
Holds **indices**. Instead of repeating a vertex for every triangle that uses it, a triangle is
three integers pointing into the vertex array. A cube face (a quad) needs only 4 vertices and 6 indices
instead of 6 full vertices.

### VAO — Vertex Array Object (`src/gl/VAO.*`)
Remembers *how to read* the VBO (the attribute layout) and *which* EBO is bound. After setup, drawing a
mesh only needs `vao.Bind(); glDrawElements(...)`.

## 3. Our vertex format

`src/geometry/Vertex.h`:

```cpp
struct Vertex {
    glm::vec3 position; // bytes  0..11  -> layout(location = 0)
    glm::vec3 normal;   // bytes 12..23  -> layout(location = 1)
    glm::vec2 uv;       // bytes 24..31  -> layout(location = 2)
};                      // stride = 32 bytes
```

The template used `position + color`. Lighting needs a **normal** (direction the surface faces) and
texturing needs a **uv** (where on the image this vertex samples), so the colour moved into the material
and the vertex gained a normal and texture coordinates.

The layout is described once in `Mesh::Mesh`:

```cpp
vao.Bind();                                                     // 1. record into this VAO
vbo.Upload(vertices.data(), vertices.size() * sizeof(Vertex));  // 2. vertex data
ebo.Upload(indices.data(), indices.size() * sizeof(GLuint));    //    index data (recorded in VAO)
vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, position));
vao.LinkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, normal));
vao.LinkAttrib(vbo, 2, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, uv));
vao.Unbind();                                                   // 3. unbind VAO BEFORE the EBO
```

`glVertexAttribPointer(location, components, type, normalized, stride, offset)`:
* **stride** — distance in bytes between the same attribute of two consecutive vertices (32).
* **offset** — where the attribute starts inside one vertex (0, 12, 24).

> Order matters: the EBO binding is stored *inside* the VAO, so the VAO must be bound when the EBO is
> bound, and must be unbound before the EBO is unbound — otherwise the VAO forgets its indices.

## 4. Shaders

`src/gl/Shader.*` loads a vertex and a fragment shader file, compiles both, links them into a program and
**checks for errors** (the template had a `compileErrors` function that was never called and compared
C-strings by pointer; it is now used and throws with the driver's log).

Extra features:
* `#include "lighting.glsl"` is expanded by our loader, so the Phong model is written once and shared
  by the Phong, Gouraud and ray tracing shaders.
* Uniform locations are cached (`glGetUniformLocation` is called once per name).

**Uniforms** are values that are constant for a whole draw call: matrices (`uModel`, `uView`, `uProj`),
material, lights. **Attributes** (`in` variables of the vertex shader) change per vertex. **Varyings**
(`out` of the vertex shader / `in` of the fragment shader) are interpolated across the triangle.

## 5. Winding order and back-face culling

OpenGL decides which side of a triangle is the *front* from the order of its vertices on the screen:
counter-clockwise (CCW) = front (`glFrontFace(GL_CCW)`, the default). All our primitives list their
vertices CCW **when seen from outside**, which is proved face by face in [03 — Primitives](03-primitives.md).

With `glEnable(GL_CULL_FACE)` the GPU skips triangles that face away from the camera — about half of all
triangles of a closed object — and we get a useful trick for free: the room's walls are single planes
facing inward, so when the camera is outside the room the near walls are back faces and disappear,
giving a doll's-house view.

## 6. Depth buffer

Each pixel stores the depth of the nearest surface drawn so far. A new fragment is kept only if it is
nearer (`GL_DEPTH_TEST`). The depth buffer is cleared together with the colour buffer every frame:
`glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`.

Transparent surfaces (Buzz's helmet, the ghost, the laser) are drawn **after** all opaque ones, sorted
far-to-near, with blending on and depth *writes* off (`glDepthMask(GL_FALSE)`), so objects behind them
remain visible.

## 7. One frame, in order (`ToyRoomApp::OnUpdate` / `OnRender`)

1. `glfwPollEvents()` — keyboard/mouse callbacks fill `Input`.
2. Delta time `dt` = seconds since the previous frame (every motion is multiplied by it).
3. Input handling → characters, camera, inspector.
4. Environment (clock, lamp, ball, ghost) and story director update.
5. Character animation (limb angles).
6. `scene->UpdateWorld()` — one depth-first pass computes every world matrix.
7. Lights copy their positions from scene nodes (lamp head, headlights…).
8. `Renderer::Collect` flattens the scene graph into a draw list.
9. Raster path: clear → opaque pass → transparent pass → debug overlay.
   Or ray tracing path: pack the draw list → trace → upscale.
10. `glfwSwapBuffers()` shows the finished image (double buffering: we always draw into the hidden
    back buffer).
