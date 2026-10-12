# OllieEngine — Project Status

*Last updated: 12 October 2026 — written for the commit "Add diffuse point lighting with LightComponent and vertex normals". Per-lesson recaps and quizzes now live in a local, git-ignored `lessons/` folder; this file keeps the project-level picture and the review list.*

This file is meant to be the fast-context doc for OllieEngine: what the engine currently does, how it's put together, and what was just changed — so a new chat (or a future you) can pick up the project without re-reading the whole codebase. Update it each time you commit: move today's "Latest Work" into the Commit Log, replace it with the new changes, and refresh the architecture snapshot if modules were added or restructured.

> **Note for Claude (or anyone helping with this project):** read this file first before answering questions about OllieEngine. It has the architecture, the most recent commit's changes, and where things stand — use it as the starting context instead of re-deriving it from scratch each session.

Each commit gets two kinds of questions: **learning questions**, aimed at building up Ollie's understanding of C++ / engine programming / 3D graphics through the code just written (since 12 Oct 2026 these live per lesson, with a recap and a marked quiz, in the local git-ignored `lessons/` folder; if Ollie says to skip an unfinished lesson, its quiz is dropped rather than carried over); and **harder review questions** (kept below), which behave more like a code review and come with a possible solution attached, for the cases worth just knowing the answer to. The suggested commit message for each session is given in chat only, not kept in this file.

> **Question-writing preferences (Ollie's feedback, 10 Sept 2026) — apply to every future set:**
> - No OpenGL/graphics-API history or "deprecated feature" trivia (e.g. core vs. compatibility profile). Skip that entirely, not useful.
> - Mix in more questions about C++ itself — syntax/semantics, memory management (ownership, lifetimes, RAII, smart vs. raw pointers) — and abstract engine-architecture design, not just OpenGL/graphics mechanics.
> - Don't ask Ollie to reason about engine features that don't exist yet (e.g. "what would you need to add for X" when X isn't implemented). Note it as a roadmap/TODO item instead of turning it into a question.
> - Close each learning-question set with a small hands-on programming task rather than another conceptual question.
> - If a question set goes unanswered before the next session, merge its still-open items into the new set rather than dropping them. (Bugs flagged in "harder questions" stay listed regardless, until they're actually fixed in code — separate from whether the quiz questions themselves got answered.)
> - Keep this file to one copy — the repo's own `PROJECT_STATUS.md`. Don't mirror it anywhere else.

## What OllieEngine is

A small custom C++ game engine, built from scratch on top of GLFW (windowing/input), GLEW (OpenGL 3.3 core function loading), and GLM (vector/matrix math), using CMake. It's a learning/hobby engine project — currently a single "engine" static-ish library (`engine/`) consumed by a sample game executable (`source/`). Assets (shaders, materials, textures, models) live in `assets/` and are loaded at runtime through the engine's `FileSystem`.

## Architecture snapshot

**`engine/source/`** — the engine library, namespace `eng`
- `Engine` (singleton) — owns the GLFW window/context, the main loop with delta time, the `InputManager`, the `GraphicsAPI`, the `FileSystem`, and the active `Scene`. Registers GLFW callbacks (plus a GLFW error callback). Each frame it pulls `CameraData` from the scene's main camera **and a `std::vector<LightData>` from `Scene::CollectLights()` `(new)`**, passes both to `RenderQueue::Draw`, swaps buffers, then resets the input manager's "old" mouse position.
- `Common.h` **`(new)`** — plain data structs shared between scene and renderer: `CameraData` (view + projection, moved here from `RenderQueue.h`) and `LightData` (colour + world position).
- `Application` — interface (`Init` / `Update` / `Destroy`) that game code implements; the engine drives it each frame.
- `input/InputManager` — keyboard state, mouse button state and old/new cursor position via GLFW callbacks.
- `io/FileSystem` — resolves the executable and `assets/` folders (CMake-injected `ASSETS_ROOT` override); `LoadAssetFile()` / `LoadAssetFileText()` take paths relative to `assets/`.
- `graphics/GraphicsAPI` — enables depth testing; `CreateShaderProgram` checks compile/link status; index/vertex buffer creation.
- `graphics/ShaderProgram` — cached uniform locations; `SetUniform` overloads for float, float pair, `glm::mat4` and **`glm::vec3` `(new)`**; `SetTexture()`.
- `graphics/Texture` — `Texture::Load(path)` via stb_image, mipmapped `GL_TEXTURE_2D`, freed in the destructor.
- `graphics/VertexLayout` — `VertexElement` + `VertexLayout`. Attribute-slot constants `positionIndex = 0`, `colourIndex = 1`, `UVIndex = 2`, **`NormalIndex = 3` `(new)`**, matching `layout (location = N)` in `vertex.glsl`.
- `render/Material` — shader + float/float2/texture params; `Material::Load()` from JSON `.mat`.
- `render/Mesh` — VAO/VBO/EBO wrapper driven by a `VertexLayout`; non-copyable. `Mesh::Load(path)` loads the first triangle primitive of a glTF (`POSITION`, `COLOR_0`, `TEXCOORD_0`, **and now `NORMAL` `(new)`**). **`Mesh::CreateCube()` `(new)`** builds the 24-vertex textured cube (position, colour, UV, normal; stride 11 floats) that used to live in `TestObject`.
- `render/RenderQueue` — collects `RenderCommand`s; `Draw(graphicsAPI, cameraData, lights)` sets `uModel`/`uView`/`uProjection` and, **if there is at least one light, `uLight.color`/`uLight.position` from `lights[0]` `(new)`**, on each command's shader.
- `scene/GameObject` — name, parent, owned children, lifecycle, position / quat rotation / scale, components. `GetWorldTransform()` composes through the parent chain; **`GetWorldPosition()` `(new)`** returns the object's world-space origin (world transform × `(0,0,0,1)`).
- `scene/Component` — base class with RTTI-free type ids (`COMPONENT(Class)`), back-pointer to its owner.
- `scene/components/MeshComponent` — `shared_ptr<Mesh>` + `shared_ptr<Material>`; submits a `RenderCommand` each `Update()`; skips silently if either is null.
- `scene/components/CameraComponent` — view matrix from position+rotation (own scale excluded) × parent world transform, inverted; perspective projection.
- `scene/components/PlayerControllerComponent` — quaternion mouse-look and WASD/up/down movement.
- `scene/components/RotatingComponent` — spins its owner around an axis.
- `scene/components/LightComponent` **`(new)`** — data-only component holding a light colour (default white). Its position comes from its owner.
- `scene/Scene` — owns root `GameObject`s; `CreateObject`, `SetParent()` with cycle check, main camera. **`CollectLights()` `(new)`** walks every object and child recursively and returns a `LightData` for each one with a `LightComponent`.

**`source/`** — the sample game executable
- `main.cpp` — creates the `Game`, initializes the `Engine` at 1280×720, runs the loop.
- `Game` — `Init()` builds the test scene: camera with controller, three brick `TestObject` cubes (two rotating, one nested), Suzanne (**now at `(0, -8, -6)`**), and **a white point light at `(0, 0, 10)` `(new)`**.
- `TestObject` — **now just `Mesh::CreateCube()` + `brick.mat` `(new)`**.

**`assets/`** — `shaders/vertex.glsl` (MVP, **plus world-space `vFragPos` and a normal transformed by `transpose(inverse(uModel))` `(new)`**), `shaders/fragment.glsl` (**diffuse lighting: `max(dot(N, L), 0) * uLight.color` multiplied into the texture colour `(new)`**), `materials/brick.mat` + `suzanne.mat`, `textures/brick.png`, `Models/suzanne/`. Copied next to the binary post-build.

**Third-party** (`engine/thirdparty/`): GLFW 3.4, GLEW, GLM 1.0.1, stb_image, nlohmann/json, cgltf.

**Build**: root `CMakeLists.txt` builds the `OllieEngine` executable and links the `Engine` library; `engine/CMakeLists.txt` lists every engine source (now including `Common.h` and `LightComponent`), uses `include_directories()` and VS `source_group` folders.

## Latest work (12 Oct 2026)

Diffuse lighting from a single point light:

- New `LightComponent` (colour) and `LightData`; `Scene::CollectLights()` gathers every light's colour and world position each frame; `Engine` passes the list to `RenderQueue::Draw`, which uploads the first light as the `uLight` struct uniform.
- Meshes now carry normals: `NormalIndex = 3` slot, `Mesh::Load` reads glTF `NORMAL`, and the hand-built cube moved into `Mesh::CreateCube()` with per-face normals.
- Vertex shader outputs world-space fragment position and normal; fragment shader computes Lambert diffuse and multiplies it into the texture colour.
- `CameraData` moved into the new `Common.h`; added `ShaderProgram::SetUniform(name, glm::vec3)` and `GameObject::GetWorldPosition()`.
- Suzanne moved to `(0, -8, -6)`; a white light placed at `(0, 0, 10)`.

## Learning questions

These now live with each lesson in `lessons/` (local only, git-ignored). The model-loading set was skipped at Ollie's request; the current lesson is `lessons/lighting-diffuse.md`.

Hands-on tasks still open across lessons:
- `GameObject::LookAt(target, up)` via `glm::quatLookAt` *(open since 24 Sept)*.

## A few harder questions (with possible solutions)

More "code review" than "learning exercise" — real gaps or bugs, each with one reasonable way to close it.

1. **`Mesh::CreateCube()` declares the normal attribute with 2 components instead of 3** *(new)*. The normal element is `{ NormalIndex, 2, GL_FLOAT, sizeof(float) * 8 }`, so the GPU reads only the normal's x and y and the shader fills z with 0. The front and back faces (normals `(0,0,±1)`) arrive as `(0,0,0)`; normalizing a zero vector is undefined, so those faces render black or garbage. The other four faces are fine, which makes it look like a lighting problem rather than a data problem.
   *Possible solution:* change the `2` to `3`. Longer term, have `VertexLayout` compute offsets and stride from element sizes (as `Mesh::Load` already does) so hand-typed layouts can't drift.

2. **The normal matrix is recomputed per vertex on the GPU** *(new)*. `mat3(transpose(inverse(uModel)))` runs a 4×4 inverse for every vertex, every frame, though it's identical for all vertices of a draw.
   *Possible solution:* compute it once per draw on the CPU (`glm::transpose(glm::inverse(glm::mat3(model)))`) and upload it as `uNormalMatrix` (needs a `mat3` `SetUniform` overload).

3. **No light means a black scene** *(new)*. If `lights` is empty, `uLight` is never set and stays zero, so every lit object renders black. Faces pointing away from the light are also pure black, since there's no ambient term yet (likely a later lesson).
   *Possible solution:* when there are no lights, upload a sensible default (e.g. a white light at the camera).

4. **`Mesh` never frees its GL objects** *(open since 10 Oct)*. No `~Mesh()`, so VAO/VBO/EBO leak.
   *Possible solution:* add `~Mesh()` calling `glDeleteVertexArrays`/`glDeleteBuffers` on non-zero handles, like `~Texture()`.

5. **Load failures are silent; Suzanne's path casing only works on Windows** *(open since 10 Oct)*. `Mesh::Load` returns `nullptr` without logging; `Game.cpp` asks for `models/suzanne/suzanne.gltf` but disk has `Models/suzanne/Suzanne.gltf`.
   *Possible solution:* log path and reason at each early return; match the casing.

6. **`Texture::Init` hard-codes `GL_RGB`** *(open since 3 Oct)*. RGBA or greyscale images would upload garbage.
   *Possible solution:* choose the format from `numChannels`; set `GL_UNPACK_ALIGNMENT` to 1.

7. **`Mesh::Load` only loads the first triangle primitive** *(by design for now)* — TODO for multi-part models.

8. **The camera's hand-built world matrix only strips its own scale, not its parent's** *(open since 24 Sept)*.
   *Possible solution:* build position+rotation only up the whole chain, or accept ancestor scale and use `GetWorldTransform()`.

9. **Direct uniform-pushing around `Material`** *(open five commits running, and growing)* — `RenderQueue::Draw` now sets the light uniforms straight on the shader too.
   *Possible solution:* route per-draw uniforms through `Material`/`Bind()`.

10. **Mouse "old" position shifted in two places per frame** *(open four commits running)* — in `cursorPositionCallback` and at the end of `Engine::Run()`.
    *Possible solution:* do the shift in exactly one place.

## Commit log

| Date | Summary |
|---|---|
| 2026-08-21 | Initial commit — engine core: `Engine` singleton (GLFW window/context, game loop), `Application` interface, `InputManager`, `GraphicsAPI`, `ShaderProgram`, `Material`, `Mesh`, vendored GLFW 3.4 + GLEW, sample triangle-rendering `Game`. |
| 2026-09-10 | Scene/GameObject hierarchy + `TestObject` sample — added `GameObject` (parent/child ownership, `IsAlive()`/`MarkForDestroy()`, cascading `Update()`), `Scene` (object storage, templated `CreateObject<T>()`, `SetParent()` with cycle checking), and `TestObject` as the first real `GameObject` (own shader/material/mesh, WASD-driven `uOffset`, submits to `RenderQueue`). |
| 2026-09-10 | Transform hierarchy via GLM — `GameObject` gained position/rotation/scale plus `GetLocalTransform()`/`GetWorldTransfrom()`; `ShaderProgram`, `RenderQueue`, and `TestObject` rewired to build and submit a `uModel` matrix instead of the old `uOffset` uniform (also fixed the frame-rate-dependent movement flagged last time). |
| 2026-09-13 | Component system + camera + full MVP pipeline — added `Component`/`MeshComponent`/`CameraComponent`; `RenderQueue`/`Engine` now build real `uView`/`uProjection` matrices from the scene's main camera each frame; fixed the `GetWorldTransfrom()` typo and its missing-return bug; reworked `engine/CMakeLists.txt`'s GLM linking. |
| 2026-09-19 | Mouse look/movement, depth testing, real shader-compile checking — added mouse tracking to `InputManager`/`Engine`, `PlayerControllerComponent` (look + move, attached to the camera), `GraphicsAPI::Init()` enabling depth testing, and real compile/link error checking in `CreateShaderProgram`. `TestObject` is now a static cube. |
| 2026-09-24 | Quaternion rotation + two bug fixes — `GameObject` rotation switched from Euler `glm::vec3` to `glm::quat`; `PlayerControllerComponent` mouse-look and `CameraComponent::GetViewMatrix()` rewritten for quaternions; fixed the `CreateIndexBuffer` size bug (open since the first commit) and the `PlayerControllerConmponent` typo. |
| 2026-10-01 | Parent/child + component test scene — added `RotatingComponent`, exercised nested transforms in `Game::Init()`, and removed `deltaTime` from mouse-look (closing the framerate-dependence review item). |
| 2026-10-03 | JSON materials, textures, and an assets pipeline — added `io::FileSystem`, `graphics::Texture` (stb_image), `ShaderProgram::SetTexture()`, texture params and JSON `Material::Load()`; `TestObject` became a UV-mapped brick cube; vendored stb_image + nlohmann/json; CMake copies `assets/` next to the binary. |
| 2026-10-10 | glTF model loading — added `Mesh::Load()` via cgltf, attribute-slot constants on `VertexElement`, Suzanne model + material spawned in `Game::Init()`; shaders simplified to a single `baseColorTexture`; `Mesh` takes indices by `const&`. |
| 2026-10-12 | Diffuse point lighting — `LightComponent`, `LightData`/`CameraData` in `Common.h`, `Scene::CollectLights()`, light uniforms in `RenderQueue::Draw`; normals in the vertex layout, glTF loader and new `Mesh::CreateCube()`; Lambert diffuse in the shaders; `GameObject::GetWorldPosition()`. |
