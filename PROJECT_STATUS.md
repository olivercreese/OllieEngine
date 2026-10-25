# OllieEngine — Project Status

*Last updated: 25 October 2026 — written for the commit "Add Bullet rigid-body physics with PhysicsComponent". Per-lesson recaps and quizzes now live in a local, git-ignored `lessons/` folder; this file keeps the project-level picture and the review list.*

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
- `Engine` (singleton) — owns the GLFW window/context, the main loop with delta time, the `InputManager`, the `GraphicsAPI`, the `FileSystem`, the `TextureManager`, **the `PhysicsManager` (`Init()` after the graphics API; `Update(deltaTime)` at the top of each frame, before `Application::Update`) `(new)`**, and the active `Scene`. `m_physicsManager` is declared before `m_currentScene`, so the scene (and its rigid bodies) is destroyed first. Registers GLFW callbacks (plus a GLFW error callback). Each frame it pulls `CameraData` from the scene's main camera (including the camera's world position) and a `std::vector<LightData>` from `Scene::CollectLights()`, passes both to `RenderQueue::Draw`, swaps buffers, then resets the input manager's "old" mouse position.
- `Common.h` — plain data structs shared between scene and renderer: `CameraData` (view + projection + `position`) and `LightData` (colour + world position).
- `Application` — interface (`Init` / `Update` / `Destroy`) that game code implements; the engine drives it each frame.
- `input/InputManager` — keyboard state, mouse button state and old/new cursor position via GLFW callbacks.
- `io/FileSystem` — resolves the executable and `assets/` folders (CMake-injected `ASSETS_ROOT` override); `LoadAssetFile()` / `LoadAssetFileText()` take paths relative to `assets/`.
- `graphics/GraphicsAPI` — enables depth testing; `CreateShaderProgram` checks compile/link status; index/vertex buffer creation. `GetDefaultShaderProgram()` lazily builds and caches a built-in shader (MVP + world-space position/normal; diffuse + Phong specular using `uLight` and `uCameraPos`) used by every glTF-loaded material.
- `graphics/ShaderProgram` — cached uniform locations; `SetUniform` overloads for float, float pair, `glm::mat4` and `glm::vec3`; `SetTexture()`.
- `graphics/Texture` — `Texture::Load(path)` via stb_image, mipmapped `GL_TEXTURE_2D`, `GL_RGB` or `GL_RGBA` by channel count, freed in the destructor. `TextureManager` caches `shared_ptr<Texture>` by path (`GetOrLoadTexture`).
- `graphics/VertexLayout` — `VertexElement` + `VertexLayout`. Attribute-slot constants `PositionIndex = 0`, `ColorIndex = 1`, `UVIndex = 2`, `NormalIndex = 3`, matching `layout (location = N)` in the shaders.
- `render/Material` — shader + float/float2/texture params; `Material::Load()` from JSON `.mat`.
- `render/Mesh` — VAO/VBO/EBO wrapper driven by a `VertexLayout`; non-copyable. **`Mesh::CreateBox(extents = vec3(1))` (was `CreateCube()`) builds a 24-vertex textured box of any size (position, colour, UV, normal; stride 11 floats); UVs stay 0–1 per face, so big boxes stretch the texture `(new)`.** `Mesh::Load` is disabled (`#if 0`), superseded by `GameObject::LoadGLTF`.
- **`physics/` `(new)`** — a thin wrapper over Bullet 3.25 (headers only included in the `.cpp`s; the `.h`s forward-declare `bt*` classes).
  - `PhysicsManager` — owns the Bullet world pieces as `unique_ptr`s, in construction order: `btDbvtBroadphase`, `btDefaultCollisionConfiguration`, `btCollisionDispatcher`, `btSequentialImpulseConstraintSolver`, `btDiscreteDynamicsWorld` (gravity −9.81 on Y). `Update` calls `stepSimulation(dt, 4, 1/60)`. `AddRigidBody` / `RemoveRigidBody` (every body is added with group `StaticFilter`, mask `AllFilter`) and set the body's `m_addedToWorld` flag.
  - `Collider` — base owning a raw `btCollisionShape*` (deleted in the destructor); `BoxCollider(extents)` (halves them for `btBoxShape`), `SphereCollider(radius)`, `CapsuleCollider(radius, height)`.
  - `RigidBody` — `BodyType { Static, Dynamic, Kinematic }`, `shared_ptr<Collider>`, mass, friction. Builds a `btRigidBody` (inertia from the shape for dynamic bodies, mass forced to 0 otherwise, kinematic flag + `DISABLE_DEACTIVATION`). `Set/GetPosition`, `Set/GetRotation` convert between GLM and Bullet. The destructor removes the body from the world if it was added.
- `render/RenderQueue` — collects `RenderCommand`s; `Draw(graphicsAPI, cameraData, lights)` sets `uModel`/`uView`/`uProjection`, `uCameraPos`, and, if there is at least one light, `uLight.color`/`uLight.position` from `lights[0]`, on each command's shader.
- `scene/GameObject` — name, parent, owned children, lifecycle, position / quat rotation / scale, components. `GetWorldTransform()` composes through the parent chain; `GetWorldPosition()` returns the world-space origin. Knows its `Scene` (`m_scene`, `GetScene()`), and `SetParent()` forwards to `Scene::SetParent`. `SetActive()` / `IsActive()`: an inactive object skips `Update()` for its components and whole subtree, so it also stops rendering. `FindChildByName(name)`: depth-first search including itself. `static LoadGLTF(path)` parses a glTF with cgltf and rebuilds `scenes[0]` as a `GameObject` tree under a `"Result"` root in the engine's current scene: one object per node (transform from `matrix` via `glm::decompose`, or TRS), a `Mesh` + `MeshComponent` per triangle primitive (`POSITION`, `COLOR_0`, `TEXCOORD_0`, `NORMAL`), and a default-shader `Material` with the base-colour texture from `TextureManager`. It also reads every glTF animation into an `AnimationClip` (translation/rotation/scale channels merged into one `TransformTrack` per node, targeted by node name) and, if there are any, adds an `AnimationComponent` to `"Result"` with each clip registered by name. The cgltf implementation lives in `GameObject.cpp`.
- `scene/Component` — base class with RTTI-free type ids (`COMPONENT(Class)`), back-pointer to its owner. **New virtual `Init()` (empty by default), called by `GameObject::AddComponent` right after `m_owner` is set; `AddComponent` now ignores `nullptr` `(new)`.**
- **`scene/components/PhysicsComponent` `(new)`** — holds a `shared_ptr<RigidBody>`. `Init()` copies the owner's world position and local rotation into the body and adds it to the world (so set the object's transform *before* adding the component). `Update()` copies position/rotation back from the body to the owner for `Dynamic` bodies only.
- `scene/components/MeshComponent` — `shared_ptr<Mesh>` + `shared_ptr<Material>`; submits a `RenderCommand` each `Update()`; skips silently if either is null.
- `scene/components/CameraComponent` — view matrix from position+rotation (own scale excluded) × parent world transform, inverted; perspective projection.
- `scene/components/PlayerControllerComponent` — quaternion mouse-look and WASD/up/down movement.
- `scene/components/RotatingComponent` — spins its owner around an axis.
- `scene/components/LightComponent` — data-only component holding a light colour (default white). Its position comes from its owner.
- `scene/components/AnimationComponent` — keyframe playback. Data structs `KeyFrameVec3`/`KeyFrameQuat` → `TransformTrack` (per target name: position/rotation/scale keys) → `AnimationClip` (name, duration, tracks). Holds clips as `shared_ptr` by name (`RegisterClip`); `Play(name, loop)` switches clip and resolves track names to child `GameObject*`s once (`BuildBindings`); `Update` advances time (wrap with `fmod` or stop) and sets local TRS using linear `glm::mix` for vectors and `glm::slerp` for rotations.
- `scene/Scene` — owns root `GameObject`s; `CreateObject` (sets each object's `m_scene`), `SetParent()` with cycle check, main camera, `CollectLights()`.

**`source/`** — the sample game executable
- `main.cpp` — creates the `Game`, initializes the `Engine` at 1280×720, runs the loop.
- `Game` — `Init()` registers the scene with the engine first (needed by `LoadGLTF`), then builds the test scene: camera with controller, three brick `TestObject` cubes (two rotating, one nested), Suzanne via `LoadGLTF` at `(0, -8, -6)`, the Sten gun via `LoadGLTF`, parented to the camera as a viewmodel at `(0.75, -0.5, -0.75)`, mirrored with scale `(-1, 1, 1)`, **with its `bullet_33` and `BOOM_35` parts hidden and the `"shoot"` clip played once, a white point light at `(0, 0, 10)`, **a static 20 × 2.02 × 20 brick `"Ground"` box at `(0, -5, 0)` with a matching `BoxCollider`, and a dynamic `TestObject` `"Falling Box"` (mass 5, tilted by the Euler angles `(1, 2, 0)` rad) dropped from `(0, 2, 2)`. The camera now starts at `(0, 1, 7)`. Sections are marked with `////` comments `(new)`**.
- `TestObject` — `Mesh::CreateBox(vec3(1))` + `brick.mat`.

**`assets/`** — `shaders/vertex.glsl` / `fragment.glsl` (diffuse only; used by `brick.mat`), `materials/brick.mat` + `suzanne.mat` (`suzanne.mat` now unused), `textures/brick.png`, `Models/suzanne/`, `Models/sten_gunmachine_carbine/` (Sketchfab glTF, 71 nodes / 31 meshes / 3 materials, animations `shoot` (0.25s) and `reload` (4.46s), all `LINEAR`; `license.txt` included). Copied next to the binary post-build.

**Third-party** (`engine/thirdparty/`): GLFW 3.4, GLEW, GLM 1.0.1 (now also `gtx/matrix_decompose`, which needs `GLM_ENABLE_EXPERIMENTAL`), stb_image, nlohmann/json, cgltf, **Bullet 3.25 (`bullet3-3.25/`, the full 343 MB / 7,600-file release; only `src/` plus the top-level CMake files are actually used) `(new)`**.

**Build**: root `CMakeLists.txt` builds the `OllieEngine` executable and links the `Engine` library; `engine/CMakeLists.txt` lists every engine source, uses `include_directories()` and VS `source_group` folders. **Bullet is pulled in with `add_subdirectory(thirdparty/bullet3-3.25)` after turning off its unit tests, demos, extras and shared libs, with `CMAKE_POLICY_VERSION_MINIMUM 3.5` (Bullet's `cmake_minimum_required` is too old for CMake 4); the engine links `LinearMath`, `BulletCollision`, `BulletDynamics`. Ollie's local `build/` was configured for Visual Studio 18 2026 with `USE_MSVC_RUNTIME_LIBRARY_DLL=ON` in the cache; that option isn't set in CMake, so a fresh configure builds Bullet with the static runtime (see review item 2) `(new)`.** `gen_proj_vs2022.bat` was edited locally for VS 2026 and added to `.gitignore`, but it's tracked, so the ignore has no effect. `engine/thirdparty/glew/glew.pc` gets rewritten with the build machine's drive letter (C: on the laptop, E: on the desktop). That's build noise, so leave it out of commits.

## Latest work (25 Oct 2026)

Rigid-body physics with Bullet:

- Vendored Bullet 3.25 and built it as a CMake subdirectory (tests/demos/extras off); the engine links `LinearMath`, `BulletCollision`, `BulletDynamics`.
- New `physics/` module: `PhysicsManager` (Bullet world, gravity, fixed-step `stepSimulation`), `Collider` + `BoxCollider` / `SphereCollider` / `CapsuleCollider`, and `RigidBody` (static / dynamic / kinematic, GLM ↔ Bullet transform conversion).
- `Engine` owns a `PhysicsManager`, initializes it after the graphics API and steps it at the start of every frame; `GetPhysicsManager()`.
- `Component::Init()` virtual hook, called from `GameObject::AddComponent` (which now ignores null).
- `PhysicsComponent`: registers its body on `Init`, copies the simulated transform back to dynamic objects every `Update`.
- `Mesh::CreateCube()` → `Mesh::CreateBox(extents)`; the stale commented `Mesh::Load` declaration is gone from `Mesh.h`.
- `Game::Init`: a static brick ground slab and a tilted dynamic box that falls onto it; camera moved back to `(0, 1, 7)`; section comments; the light object renamed from `"Ligth"` to `"light"`.

## Learning questions

These now live with each lesson in `lessons/` (local only, git-ignored; each machine has its own copy, and the desktop's copy was started 17 Oct). The current lesson is `lessons/bullet-physics.md` (written on the laptop, whose `lessons/` only holds `model-loading.md` and `lighting-diffuse.md`; the other lessons are on the desktop). On 25 Oct Ollie said to skip the unanswered `specular-and-gltf-scenes` and `gltf-animation` quizzes and move on, so they are not merged into the new set.

Hands-on tasks still open across lessons:
- Spawn a new dynamic box above the ground each time a key is pressed *(new, 25 Oct)*.
- Play `"shoot"` on a left-click press; bonus: show `BOOM_35` only while it plays *(open since 18 Oct)*.
- In `ParseGLTFNode`, add the `MeshComponent` outside the `if (primitive.material)` block and guard a null `node->name` *(open since 17 Oct)*.
- Parent the light under a rotating object so it orbits *(open since 12 Oct)*.
- `GameObject::LookAt(target, up)` via `glm::quatLookAt` *(open since 24 Sept)*.

## A few harder questions (with possible solutions)

More "code review" than "learning exercise" — real gaps or bugs, each with one reasonable way to close it. Items 1–8 are new on 25 Oct; the rest were open before.

*Closed 25 Oct:* the commented `Mesh::Load` declaration in `Mesh.h` is gone (the `#if 0` body and the rest of item 23 remain).

*Closed 18 Oct:* none (this commit added animation without touching the open items).

*Closed 17 Oct:* cube normal had 2 components (fixed to 3); `Mesh::Load` only loaded the first primitive (`LoadGLTF` loads every primitive of every node); Suzanne path casing (`Suzanne.gltf` now matches; the `models/` vs `Models/` folder casing still differs).

1. **Every `btDefaultMotionState` leaks.** `RigidBody`'s constructor does `new btDefaultMotionState(transform)` and hands the raw pointer to Bullet, but `btRigidBody` doesn't own its motion state, and nothing deletes it.
   *Possible solution:* keep a `std::unique_ptr<btDefaultMotionState> m_motionState` member, declared *before* `m_body` so it outlives the body during destruction.

2. **A fresh configure will probably fail to link (MSVC runtime mismatch).** Bullet's `USE_MSVC_RUNTIME_LIBRARY_DLL` defaults to `OFF`, and then Bullet rewrites `/MD` to `/MT` for its own libraries while the engine stays `/MD` → `LNK2038 RuntimeLibrary mismatch`. The laptop's `build/CMakeCache.txt` has it `ON`, set by hand, so it works there but not on a clean clone or the desktop. Separately, `option(BUILD_UNIT_TESTS OFF)` passes `OFF` as the *help text*; it only ends up off because `option()` defaults to `OFF`.
   *Possible solution:* before `add_subdirectory`, `set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)` and the same `set(<name> OFF CACHE BOOL "" FORCE)` form for the other switches; also `BUILD_BULLET3 OFF` to skip the unused Bullet3/OpenCL/SoftBody libraries.

3. **343 MB of Bullet in the repo.** `bullet3-3.25/` is the full release (7,600 files: 207 MB `examples/`, 105 MB `data/`, 14 MB `docs/`); with demos/tests/extras off, the build only reads `src/` (9.5 MB) and the top-level CMake files. Every clone carries the rest forever once it's committed.
   *Possible solution:* before committing, delete (or ignore) `examples/`, `data/`, `docs/`, `test/`, `Extras/`, `build3/`; or use a git submodule / CMake `FetchContent` instead of vendoring.

4. **Physics and the scene only sync one way, and only at the top level.** Static and kinematic bodies are placed once in `Init()` and never follow their `GameObject` again (a kinematic body is supposed to be driven by game code every frame). Dynamic bodies write a *world* position into the owner's *local* `SetPosition`, which is wrong as soon as the object has a parent. `Init()` also mixes world position with *local* rotation.
   *Possible solution:* in `Update`, for kinematic bodies push the owner's world transform into the body (via the motion state); for dynamic bodies, convert world → parent-local before setting, or document that physics objects must be roots.

5. **Collider size is independent of the object's scale.** The ground's `groundExtents` has to be passed to both `CreateBox` and `BoxCollider`, and `SetScale` on a physics object changes the mesh but not the collider.
   *Possible solution:* apply the owner's scale with `shape->setLocalScaling(...)` in `PhysicsComponent::Init` (and keep extents at 1), or build both mesh and collider from one size.

6. **No interpolation, and long frames lose time.** `stepSimulation(dt, 4, 1/60)` runs fixed 60 Hz steps and leaves the remainder for the motion state to interpolate, but `RigidBody::GetPosition/GetRotation` read `getWorldTransform()` (the last whole step), so on a high-refresh monitor the box moves in visible steps. A frame longer than 4/60 s (e.g. the first frame after loading the gun) drops the extra time, so physics briefly runs in slow motion.
   *Possible solution:* read through `m_body->getMotionState()->getWorldTransform(tr)` for dynamic bodies; clamp or log very large `deltaTime`.

7. **`Collider` owns a raw pointer but is copyable.** The destructor deletes `m_shape`; a copy of a `BoxCollider` would delete the same shape twice. (`if (m_shape) delete m_shape;` also doesn't need the check: deleting `nullptr` is a no-op.)
   *Possible solution:* `std::unique_ptr<btCollisionShape>` (fine with a forward declaration as long as the destructor is defined in the `.cpp`, which it is), which also makes the class move-only automatically.

8. **`RigidBody` getters assume a body exists.** The constructor returns early on a null collider, leaving `m_body` empty, but `GetPosition()`/`GetRotation()` dereference it unconditionally (the setters do check). `m_mass` and `m_friction` are stored but never read again. All bodies are also added with collision group `StaticFilter`, even dynamic ones; it works because the mask is `AllFilter`, but any future filtering by group will misbehave.
   *Possible solution:* guard the getters; drop the unused members or add getters; call `m_world->addRigidBody(body)` (Bullet picks `StaticFilter` or `DefaultFilter` from the body's flags) or choose the group from `BodyType`.

9. **One-shot clips stop one frame short.** In `AnimationComponent::Update`, when `m_time > duration` and not looping, it resets and returns before sampling, so the object freezes on the previous frame's pose rather than the clip's final keys. A `duration` of 0 would also `fmod` by zero.
   *Possible solution:* clamp `m_time` to `duration`, apply that pose, then stop; skip the wrap when `duration <= 0`.

10. **Interpolation mode is ignored.** Every sampler is treated as `LINEAR`. `STEP` gets smoothed, and `CUBICSPLINE` outputs hold 3 values per key (in-tangent, value, out-tangent), so `values[i]` would be wrong. The Sten's clips are all `LINEAR`, so it's latent.
   *Possible solution:* store `sampler->interpolation` on the track; for `STEP` return `keys[i0].value`, for `CUBICSPLINE` read every third value (or evaluate the Hermite spline).

11. **Raw pointers that can dangle.** `m_clip` points into a clip owned by `m_clips`; `RegisterClip` with the playing clip's name frees it. `m_bindings` holds `GameObject*`s that die if a child is destroyed.
   *Possible solution:* hold `std::shared_ptr<AnimationClip>` for the current clip; rebuild bindings (or check `IsAlive()`) when the hierarchy changes.

12. **Quaternion `Interpolate` returns `glm::vec3(0.0f)` for empty keys.** It compiles via `glm::quat`'s implicit Euler-angle constructor and happens to give identity.
   *Possible solution:* `return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);`.

13. **`AnimationComponent` loose ends.** `AnimationClip::looping` is never read; `ObjectBinding::object` duplicates the map key and the `unique_ptr<ObjectBinding>` adds an allocation per object for no gain; the `orentation` typo is back in `ParseGLTFNode`; key lookup is a linear scan per frame (fine at 6 keys, slower for the 71-key `reload`).
   *Possible solution:* drop `looping` or use it as `Play`'s default; `unordered_map<GameObject*, std::vector<size_t>>`; fix the spelling; remember the last key index and search forward from it.

14. **Primitives without a material are dropped**. In `ParseGLTFNode`, `object->AddComponent(new MeshComponent(mesh, mat))` is inside `if (primitive.material)`, so a primitive with no material builds a mesh and discards it.
   *Possible solution:* add the component after the material block; the default-shader material works untextured.

15. **Null `node->name` crashes**. glTF names are optional; `CreateObject(node->name, …)` constructs a `std::string` from `nullptr`. The animation loader's `track.targetName = node->name` has the same problem.
   *Possible solution:* `node->name ? node->name : "Node"`.

16. **`LoadGLTF` fails silently and `Game.cpp` doesn't check**. Every early return gives `nullptr` with no log, and `gun->SetParent(camera)` dereferences it immediately. `data->scenes[0]` is read without checking `scenes_count` (and ignores `data->scene`, the file's default scene).
   *Possible solution:* log the path and reason at each early return; use `data->scene ? data->scene : &data->scenes[0]` after checking `scenes_count`; null-check results in `Game::Init`.

17. **`LoadGLTF` uses the engine's current scene implicitly**. It calls `Engine::GetInstance().GetScene()`, which forced `SetScene` to the top of `Game::Init`. That's a hidden ordering dependency, and you can't load into a scene that isn't active.
   *Possible solution:* take a `Scene&` parameter (or make it `Scene::LoadGLTF`).

18. **Alpha-mask materials ignored**. Two of the Sten's materials are `alphaMode: MASK`, but the shader never discards, so cut-out texels draw solid.
    *Possible solution:* read `alphaMode`/`alphaCutoff` into the material and `if (texColor.a < uAlphaCutoff) discard;`.

19. **Two lighting shaders have diverged**. The built-in default shader has specular; `assets/shaders/fragment.glsl` (bricks) doesn't, so the same light looks different on different objects. Keeping GLSL in C++ strings also means no hot-editing.
    *Possible solution:* make the default shader load the asset shader files (or bring `fragment.glsl` up to date and share it).

20. **Specular on faces turned away from the light**. `spec` isn't gated on `diff > 0`, so back-facing surfaces can catch highlights at grazing angles.
    *Possible solution:* `if (diff > 0.0)` around the specular term (or multiply by `step(0.0, dot(norm, lightDir))`).

21. **`ParseGLTFNode` has external linkage**. It's a free function at namespace scope in `GameObject.cpp` with no header declaration; another `.cpp` defining the same signature would be an ODR violation / link error. The new `ReadScalar`/`ReadVec3`/… lambdas are namespace-scope variables in the same file with the same issue.
    *Possible solution:* mark it `static` or put it in an anonymous namespace.

22. **Negative scale flips winding** *(latent)*. The gun's `(-1, 1, 1)` scale reverses triangle winding; fine now, but if `GL_CULL_FACE` is enabled it will render inside-out.
    *Possible solution:* when the model matrix determinant is negative, flip `glFrontFace` for that draw, or mirror the asset instead.

23. **Dead code**. `Mesh::Load` under `#if 0`, the commented cgltf include in `Mesh.cpp`, and the unused `suzanne.mat`.
    *Possible solution:* delete; git history keeps it.

24. **The normal matrix is recomputed per vertex on the GPU** *(open since 12 Oct; now in both shaders)*.
    *Possible solution:* compute `glm::transpose(glm::inverse(glm::mat3(model)))` once per draw on the CPU and upload `uNormalMatrix`.

25. **No light means a black scene** *(open since 12 Oct)*. Empty `lights` leaves `uLight` zero; no ambient term yet.
    *Possible solution:* upload a default light when none exist.

26. **`Mesh` never frees its GL objects** *(open since 10 Oct, and now matters more: the gun creates 31 meshes)*.
    *Possible solution:* `~Mesh()` calling `glDeleteVertexArrays`/`glDeleteBuffers`, like `~Texture()`.

27. **`Texture` still mishandles 1- and 2-channel images** *(narrowed 17 Oct)*. RGB/RGBA are handled now, but grey or grey+alpha images (e.g. the unused `material_0_baseColor2.png`) still upload as `GL_RGB`, and odd row widths need `GL_UNPACK_ALIGNMENT` 1.
    *Possible solution:* map 1 → `GL_RED`, 2 → `GL_RG` (with swizzle), or ask stb_image for 4 channels always; set unpack alignment to 1.

28. **`TextureManager` never releases anything**. It holds a `shared_ptr` forever, so textures live until the engine dies; a failed load also caches `nullptr` for that path.
    *Possible solution:* fine for now; later, store `weak_ptr` or add an explicit unload/garbage-collect pass.

29. **The camera's hand-built world matrix only strips its own scale, not its parent's** *(open since 24 Sept)*.
    *Possible solution:* build position+rotation only up the whole chain, or accept ancestor scale and use `GetWorldTransform()`.

30. **Direct uniform-pushing around `Material`** *(open seven commits running)* — `uCameraPos` joined the light uniforms in `RenderQueue::Draw`.
    *Possible solution:* route per-draw uniforms through `Material`/`Bind()`.

31. **Mouse "old" position shifted in two places per frame** *(open six commits running)*.
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
| 2026-10-17 | glTF scenes + specular — `GameObject::LoadGLTF()` rebuilds a glTF node tree as `GameObject`s (every primitive, node transforms, base-colour textures) and replaces `Mesh::Load`; `TextureManager` cache; RGBA textures; built-in default shader with Phong specular and `uCameraPos`; `GameObject` knows its `Scene`; Sten gun viewmodel on the camera; cube normal count fixed. |
| 2026-10-18 | glTF keyframe animation — `AnimationComponent` (clips, per-node TRS tracks, lerp/slerp, loop or one-shot), animations read in `LoadGLTF` and attached to the loaded root; `GameObject::SetActive`/`IsActive` and `FindChildByName`; the gun hides its bullet/flash and plays `shoot` on start. |
| 2026-10-25 | Bullet rigid-body physics — vendored Bullet 3.25 as a CMake subdirectory; `PhysicsManager` (world, gravity, fixed-step simulation) owned by `Engine`; `Collider` (box/sphere/capsule) and `RigidBody` (static/dynamic/kinematic); `PhysicsComponent` syncing dynamic bodies back to their object; `Component::Init()` hook called from `AddComponent`; `Mesh::CreateCube` → `CreateBox(extents)`; a falling box lands on a static ground slab. |
