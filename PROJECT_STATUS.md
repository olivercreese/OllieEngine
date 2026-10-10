# OllieEngine — Project Status

*Last updated: 10 October 2026 — written for the commit "Add glTF model loading via cgltf and render Suzanne". (This update also catches up on the 3 Oct materials/textures commit, which didn't update this file.)*

This file is meant to be the fast-context doc for OllieEngine: what the engine currently does, how it's put together, and what was just changed — so a new chat (or a future you) can pick up the project without re-reading the whole codebase. Update it each time you commit: move today's "Latest Work" into the Commit Log, replace it with the new changes, and refresh the architecture snapshot if modules were added or restructured.

> **Note for Claude (or anyone helping with this project):** read this file first before answering questions about OllieEngine. It has the architecture, the most recent commit's changes, and where things stand — use it as the starting context instead of re-deriving it from scratch each session.

Each commit gets two kinds of questions below: **learning questions**, aimed at building up Ollie's understanding of C++ / engine programming / 3D graphics through the code just written (deliberately open — the point is to work them out, not read an answer); and **harder review questions**, which behave more like a code review and come with a possible solution attached, for the cases worth just knowing the answer to. The suggested commit message for each session is given in chat only, not kept in this file.

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
- `Engine` (singleton) — owns the GLFW window/context, the main loop with delta time, the `InputManager`, the `GraphicsAPI`, the `FileSystem`, and the active `Scene`. Registers GLFW callbacks for keyboard, mouse buttons, and cursor position. Each frame it pulls `CameraData` (view + projection) from the scene's main camera, draws, swaps buffers, then resets the input manager's "old" mouse position to the current one.
- `Application` — interface (`Init` / `Update` / `Destroy`) that game code implements; the engine drives it each frame.
- `input/InputManager` — polls keyboard state via GLFW callbacks; also tracks mouse button state and old/new cursor position (for computing per-frame mouse deltas).
- `io/FileSystem` — resolves the executable and `assets/` folders (with a CMake-injected `ASSETS_ROOT` override via `config.h.in`); `LoadAssetFile()` (bytes) and `LoadAssetFileText()` (string) take paths relative to `assets/`.
- `graphics/GraphicsAPI` — `Init()` enables depth testing. `CreateShaderProgram` checks compile/link status, logs GL errors, returns `nullptr` on failure. `CreateIndexBuffer` sizes its upload as `indices.size() * sizeof(uint32_t)`.
- `graphics/ShaderProgram` — compiled shader program with cached uniform locations; `SetUniform` has float, float-pair, and `glm::mat4` overloads; `SetTexture()` binds a `Texture` to a sampler uniform.
- `graphics/Texture` — `Texture::Load(path)` reads an image with stb_image and uploads it as a mipmapped `GL_TEXTURE_2D`; deletes its GL handle in the destructor.
- `graphics/VertexLayout` — `VertexElement` (attribute index, component count, GL type, byte offset) + `VertexLayout` (element list + stride). **`VertexElement` now has `positionIndex = 0`, `colourIndex = 1`, `UVIndex = 2` constants `(new)`** — the agreed attribute slots that both the glTF loader and `layout (location = N)` in `vertex.glsl` use.
- `render/Material` — binds a `ShaderProgram` and holds float/float2/texture params. `Material::Load(path)` parses a JSON `.mat` file (shader paths + params, including textures) and builds the whole thing.
- `render/Mesh` — VAO/VBO/EBO wrapper driven by a data-defined `VertexLayout`; non-copyable. **`Mesh::Load(path)` `(new)`** parses a `.gltf` with cgltf, loads its `.bin` buffers from the model's folder, and builds a `Mesh` from the *first* triangle primitive it finds: `POSITION`, `COLOR_0` and `TEXCOORD_0` are mapped to attribute slots 0/1/2 and interleaved into one float array; uses indices if the primitive has them. Index-buffer constructor now takes the index vector by `const&`.
- `render/RenderQueue` — collects `RenderCommand`s (material + mesh + modelMatrix); `Draw()` takes a `CameraData` and sets `uModel`, `uView`, and `uProjection` on each command's shader before drawing.
- `scene/GameObject` — name, parent pointer, owned children, `IsAlive()`/`MarkForDestroy()` lifecycle, position / `glm::quat` rotation / scale, and a component list (`AddComponent()` / templated `GetComponent<T>()`). `GetWorldTransform()` recursively composes through the parent chain.
- `scene/Component` — base class for per-object behaviour/data, with RTTI-free per-type ids via `Component::StaticTypeId<T>()` and the `COMPONENT(Class)` macro. Holds a back-pointer (`m_owner`) to its `GameObject`.
- `scene/components/MeshComponent` — holds a `shared_ptr<Mesh>` + `shared_ptr<Material>`; every `Update()` submits a `RenderCommand` (owner's world transform). Silently skips if either is null.
- `scene/components/CameraComponent` — `GetViewMatrix()` hand-builds a position+rotation matrix (excluding the camera's own scale), multiplies in the parent's world transform if any, then inverts. Perspective projection with configurable FOV/near/far.
- `scene/components/PlayerControllerComponent` — first-person controller: quaternion mouse-look (yaw around world up, pitch around local right; no longer scaled by `deltaTime`) and WASD/up/down movement relative to view direction.
- `scene/components/RotatingComponent` — spins its owner around a given axis at a given speed.
- `scene/Scene` — owns the root set of `GameObject`s. `CreateObject`/templated `CreateObject<T>()` (optionally with a parent); `SetParent()` reparents with a cycle check; `SetMainCamera()`/`GetMainCamera()`; `Clear()`.

**`source/`** — the sample game executable
- `main.cpp` — creates the `Game`, initializes the `Engine` at 1280×720, runs the loop.
- `Game` — implements `Application`; owns one `eng::Scene* m_scene`. `Init()` builds a small test scene: a camera with `CameraComponent` + `PlayerControllerComponent`, three textured `TestObject` cubes (two rotating, one nested), and **a Suzanne `GameObject` at the origin whose `MeshComponent` gets `Mesh::Load("models/suzanne/suzanne.gltf")` and `Material::Load("materials/suzanne.mat")` `(new)`**.
- `TestObject` — a `GameObject` subclass that builds a UV-mapped cube mesh and loads its material from `assets/materials/brick.mat`.

**`assets/`** — `shaders/vertex.glsl` + `fragment.glsl` (now just MVP + a single `baseColorTexture` sample; the per-vertex colour was dropped), `materials/brick.mat` + `suzanne.mat`, `textures/brick.png`, `Models/suzanne/` (Suzanne `.gltf` + `.bin` + base-colour and metallic-roughness PNGs). Copied next to the built binary by a post-build step.

**Third-party** (`engine/thirdparty/`): GLFW 3.4, GLEW, GLM 1.0.1, stb_image, nlohmann/json, and **cgltf `(new)`** (header-only; `CGLTF_IMPLEMENTATION` is defined in `Mesh.cpp`).

**Build**: root `CMakeLists.txt` builds the `OllieEngine` executable from `source/*` and links the `Engine` library. `engine/CMakeLists.txt` now uses plain `include_directories()` for every dependency (instead of `target_include_directories(... PUBLIC ...)`) and groups source files into folders for the Visual Studio solution via `source_group`.

## Latest work (10 Oct 2026)

Model loading — the engine can now load a mesh from a glTF file instead of only hand-typed vertex arrays:

- Added `Mesh::Load()` using cgltf: parse the `.gltf` JSON, load the binary buffers relative to the model's folder, walk meshes → primitives, build a `VertexLayout` from the primitive's attributes, read every vertex's attributes into one interleaved `std::vector<float>`, read indices into a `std::vector<uint32_t>`, and hand both to the existing `Mesh` constructors.
- Added attribute-slot constants to `VertexElement` so the loader writes position/colour/UV into the same locations the vertex shader reads from.
- `Mesh` index constructor takes indices by `const&` (was by value — a needless copy); `Draw()` casts counts to `GLsizei` explicitly.
- Shaders: dropped the per-vertex colour (Suzanne's glTF has no `COLOR_0`), renamed the sampler `brickTexture` → `baseColorTexture` so one shader serves both materials; `brick.mat` updated to match, `suzanne.mat` added.
- `Game::Init()` spawns Suzanne at the origin.
- Vendored cgltf; tidied `engine/CMakeLists.txt` to match the course's layout.
- *Not lesson work:* a `glfwSetErrorCallback` that prints GLFW errors to `std::cerr` was added to `Engine::Init()` in a separate debugging session. Commit it on its own if you want to keep it.

## Learning questions from this commit — model loading, C++, and engine architecture

Grounded in `Mesh::Load()` and the shader/material changes. Work them through by hand.

1. `Mesh::Load` returns `std::shared_ptr<Mesh>`, and `Mesh` deletes its copy constructor and copy assignment. Why does that combination make sense for an object that owns GPU buffer handles — what would go wrong if two `Mesh` copies thought they owned the same VAO?
2. `cgltf_free(data)` has to run on every path out of the function once `cgltf_parse` has succeeded. Trace every `return` in `Mesh::Load` — is each one covered? Then: which C++ idiom would make forgetting it impossible?
3. The loader stores accessors in `accessors[3]` indexed by `VertexElement::positionIndex`/`colourIndex`/`UVIndex`, not by the order attributes appear in the file. Why does that matter, and how do those constants connect to `layout (location = 0/1/2)` in `vertex.glsl`?
4. `cgltf_load_buffers` is given `fullPath.remove_filename()`. What lives in `Suzanne.gltf` versus `Suzanne.bin`, and why does the loader need the model's folder rather than just the `.gltf` path you passed to `Mesh::Load`?
5. `readFloats` and `readIndex` are lambdas stored in `auto` variables with an empty capture list `[]`. What type does `auto` deduce there, why don't they need to capture anything, and what is the `std::fill` at the top of `readFloats` protecting against?
6. **Programming task:** replace the manual `cgltf_free(data)` calls with a `std::unique_ptr<cgltf_data, decltype(&cgltf_free)>` (constructed right after `cgltf_parse` succeeds), so cleanup happens automatically on every return path. Then delete the now-redundant `cgltf_free` lines.
7. **Still open from 24 Sept — programming task:** add `GameObject::LookAt(const glm::vec3& target, const glm::vec3& up = {0,1,0})` that sets `m_rotation` so the object faces `target` (`glm::quatLookAt` gets you there). Try it by making Suzanne always face the camera.

## A few harder questions (with possible solutions)

More "code review" than "learning exercise" — real gaps or bugs, each with one reasonable way to close it.

1. **`Mesh` never frees its GL objects** *(new)*. There's no `~Mesh()`, so the VAO/VBO/EBO created in the constructors leak when the last `shared_ptr<Mesh>` goes away. `Texture` already does this correctly in its destructor.
   *Possible solution:* add `~Mesh()` that calls `glDeleteVertexArrays`/`glDeleteBuffers` on any non-zero handle — the same pattern as `~Texture()`.

2. **Load failures are completely silent** *(new)*. `Mesh::Load` returns `nullptr` on every failure without logging, and `MeshComponent::Update` quietly skips a null mesh — so a typo in a path just means "Suzanne doesn't appear". Related: `Game.cpp` asks for `models/suzanne/suzanne.gltf` but the folder/file on disk are `Models/suzanne/Suzanne.gltf`. That only works because Windows paths are case-insensitive; on Linux/macOS it would fail with no message.
   *Possible solution:* log the path and the `cgltf_result` (or "file not found") at each early return; make the path in `Game.cpp` match the on-disk casing (or rename the folder to lowercase `models`).

3. **`Texture::Init` hard-codes `GL_RGB`** *(new — from the 3 Oct commit)*. `numChannels` from stb_image is stored but ignored, so an RGBA or greyscale PNG would upload garbage. All current PNGs happen to be RGB. Separately, RGB rows whose byte width isn't a multiple of 4 need `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)`.
   *Possible solution:* pick the format from `numChannels` (1 → `GL_RED`, 3 → `GL_RGB`, 4 → `GL_RGBA`) and set the unpack alignment to 1 before `glTexImage2D`.

4. **`Mesh::Load` only ever loads the first triangle primitive** *(new, by design for now)*. Multi-mesh or multi-material models will be missing parts. Not a bug for Suzanne — note as a TODO for when a model needs it.

5. **The camera's hand-built world matrix only strips its own scale, not its parent's** *(open since 24 Sept)*. `GetViewMatrix()` still multiplies in `GetParent()->GetWorldTransform()`, scale and all.
   *Possible solution:* unchanged — either build position+rotation only up the whole parent chain, or accept ancestor scale and just use `GetWorldTransform()`.

6. **Direct uniform-pushing around `Material`** *(open four commits running)* — `RenderQueue::Draw` still sets `uModel`/`uView`/`uProjection` straight on the shader program.
   *Possible solution:* unchanged — `glm::mat4` overload for `Material::SetParam`, route per-draw uniforms through `Bind()`.

7. **Mouse "old" position still gets shifted in two places per frame** *(open three commits running)* — in `cursorPositionCallback` and again at the end of `Engine::Run()`.
   *Possible solution:* unchanged — do the shift in exactly one place.

*Closed since last update:* mouse-look framerate dependence — `* deltaTime` was removed from both look angles in the 1 Oct parent/child commit.

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
