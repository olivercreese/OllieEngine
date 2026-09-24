# OllieEngine — Project Status

*Last updated: 24 September 2026 — committed and pushed as "Switch rotation to quaternions; fix index buffer size and PlayerController typo".*

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

A small custom C++ game engine, built from scratch on top of GLFW (windowing/input), GLEW (OpenGL 3.3 core function loading), and GLM (vector/matrix math), using CMake. It's a learning/hobby engine project — currently a single "engine" static-ish library (`engine/`) consumed by a sample game executable (`source/`).

## Architecture snapshot

**`engine/source/`** — the engine library, namespace `eng`
- `Engine` (singleton) — owns the GLFW window/context, the main loop with delta time, the `InputManager`, the `GraphicsAPI`, and the active `Scene`. Registers GLFW callbacks for keyboard, mouse buttons, and cursor position. Each frame it pulls `CameraData` (view + projection) from the scene's main camera, draws, swaps buffers, then resets the input manager's "old" mouse position to the current one.
- `Application` — interface (`Init` / `Update` / `Destroy`) that game code implements; the engine drives it each frame.
- `input/InputManager` — polls keyboard state via GLFW callbacks; also tracks mouse button state and old/new cursor position (for computing per-frame mouse deltas).
- `graphics/GraphicsAPI` — `Init()` enables depth testing. `CreateShaderProgram` checks vertex/fragment compile status and program link status, logs GL errors, and returns `nullptr` on failure. `CreateIndexBuffer` now correctly sizes its upload as `indices.size() * sizeof(uint32_t)` **(fixed this commit — was `sizeof(float)`, open since the first commit)**.
- `graphics/ShaderProgram` — compiled shader program with cached uniform locations; `SetUniform` has float, float-pair, and `glm::mat4` overloads.
- `render/Material` — binds a `ShaderProgram` and holds float/float2 uniform params; already no-ops safely in `Bind()` if the shader is null.
- `render/Mesh` — VAO/VBO/EBO wrapper driven by a data-defined `VertexLayout`.
- `render/RenderQueue` — collects `RenderCommand`s (material + mesh + modelMatrix); `Draw()` takes a `CameraData` and sets `uModel`, `uView`, and `uProjection` on each command's shader before drawing.
- `scene/GameObject` — name, parent pointer, owned children, `IsAlive()`/`MarkForDestroy()` lifecycle, position/scale, and a component list (`AddComponent()` / templated `GetComponent<T>()`). **Rotation is now stored as a `glm::quat` instead of Euler-angle `glm::vec3` `(new)`**, and `GetLocalTransform()` composes it with `glm::mat4_cast(m_rotation)` instead of three separate axis rotations. `GetWorldTransform()` still recursively composes through the parent chain.
- `scene/Component` — base class for per-object behaviour/data, with RTTI-free per-type ids via `Component::StaticTypeId<T>()` and the `COMPONENT(Class)` macro. Holds a back-pointer (`m_owner`) to its `GameObject`.
- `scene/components/MeshComponent` — owns a `Mesh` + `Material` pair; every `Update()` submits a `RenderCommand` (using the owner's world transform) to the `RenderQueue`.
- `scene/components/CameraComponent` — **`GetViewMatrix()` reworked `(new)`**: instead of `glm::inverse(m_owner->GetWorldTransform())`, it now hand-builds a rotation-and-position-only matrix (`glm::mat4_cast(rotation)` with the translation column overwritten by position — deliberately excluding the camera's own scale), multiplies in the immediate parent's full `GetWorldTransform()` if there is one, then inverts. `GetProjectionMatrix()` unchanged (perspective, configurable FOV/near/far).
- `scene/components/PlayerControllerComponent` **(renamed this commit — was misspelled `PlayerControllerConmponent`)** — first-person-style controller: left-mouse-drag look and WASD move relative to view direction. **Look is now quaternion-based `(new)`**: yaw is applied around the world's fixed up axis, pitch around the object's current local right axis, combined and pre-multiplied onto the existing rotation, then renormalized; `front`/`right` for movement come from rotating the unit axes by the current rotation (`rotation * glm::vec3(...)`) instead of building a rotation matrix by hand.
- `scene/Scene` — owns the root set of `GameObject`s. `CreateObject`/templated `CreateObject<T>()`; `SetParent()` reparents with a cycle check; `SetMainCamera()`/`GetMainCamera()`; `Clear()`.

**`source/`** — the sample game executable
- `main.cpp` — creates the `Game`, initializes the `Engine` at 1280×720, runs the loop.
- `Game` — implements `Application`; owns one `eng::Scene* m_scene`. `Init()` creates a camera `GameObject` with both `CameraComponent` and `PlayerControllerComponent`, positions it, sets it as the scene's main camera, and spawns one `TestObject`.
- `TestObject` — a `GameObject` subclass that builds a cube mesh/material/shader in the constructor and hands them to a `MeshComponent`. Doesn't move itself — WASD/mouse control lives on the camera.

**Third-party** (`engine/thirdparty/`): GLFW 3.4, GLEW, and GLM 1.0.1, all vendored and linked via `engine/CMakeLists.txt`.

**Build**: root `CMakeLists.txt` builds the `OllieEngine` executable from `source/*` and links the `Engine` library, which pulls in GLFW/GLEW/GLM.

## Latest work (committed 24 Sept 2026)

Switched rotation from Euler angles to quaternions throughout, plus two long-open review bugs fixed:

- `GameObject::m_rotation` is now a `glm::quat` (was `glm::vec3` Euler angles); `GetLocalTransform()` composes it with `glm::mat4_cast` instead of three chained axis rotations.
- `PlayerControllerComponent`'s mouse-look rewritten for quaternions: yaw around world-up, pitch around the object's current local right, combined via quaternion multiplication and renormalized; movement direction vectors now come from rotating unit axes by the current rotation instead of building a rotation matrix by hand.
- `CameraComponent::GetViewMatrix()` reworked to hand-build a rotation+position matrix (excluding the camera's own scale) rather than calling `GetWorldTransform()` directly.
- Fixed `GraphicsAPI::CreateIndexBuffer`'s buffer-size bug (`sizeof(uint32_t)` instead of `sizeof(float)`) — **closes the harder-question open since the very first commit.**
- Fixed the `PlayerControllerConmponent` → `PlayerControllerComponent` typo across the header, .cpp, and `Game.cpp` — **closes last commit's naming nitpick.**

## Learning questions from this commit — quaternions, C++, and engine architecture

Grounded in the actual code from this commit (`GameObject`, `PlayerControllerComponent`, `CameraComponent`). Worth working through by hand rather than just reading an answer.

1. `GameObject` switched from Euler angles to quaternions, and `GetLocalTransform()` now just does `mat * glm::mat4_cast(m_rotation)`. What specific problem with Euler angles does a quaternion avoid — and why does the new mouse-look code need `glm::normalize(deltaRot * rotation)` when the old Euler-angle version never needed anything like a normalize step?
2. `PlayerControllerComponent` computes `front`/`right` as `rotation * glm::vec3(0,0,-1)` and `rotation * glm::vec3(1,0,0)`. In plain terms, what is `operator*` doing when you multiply a `glm::quat` by a `glm::vec3` — and why does this replace the old code's need for a whole rotation matrix just to get two direction vectors?
3. The new mouse-look pitches around the object's *current local* right axis (`rotation * glm::vec3(1,0,0)`) but yaws around the world's *fixed* up axis, rather than the object's own local up. Why does mixing "local" for one axis and "world" for the other give you the familiar "can't tip over" FPS-camera feel, instead of a free-spinning camera?
4. `CameraComponent::GetViewMatrix()` now builds its own position+rotation matrix by hand instead of calling `m_owner->GetWorldTransform()` like it used to. What does this version deliberately leave out compared to `GetWorldTransform()`, and why might that be the right call specifically for a camera?
5. **Programming task:** add a `LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f))` method to `GameObject` that computes and sets `m_rotation` so the object faces `target` from its current position (GLM's `glm::quatLookAt`, or building a matrix with `glm::lookAt` and extracting the rotation, will get you there). Use it to make the camera always face the cube no matter where either one moves.

## A few harder questions (with possible solutions)

More "code review" than "learning exercise" — real gaps or bugs, each with one reasonable way to close it.

1. **Mouse-look is still framerate-dependent — it survived the quaternion rewrite unfixed.** `float yAngle = -deltaX * m_sensitivity * deltaTime;` (and the `.x`/pitch equivalent) still multiply an already-elapsed mouse-pixel delta by `deltaTime`, so the same physical mouse swipe turns the camera by a different amount depending on framerate. This was flagged last session, and the surrounding code got fully rewritten for quaternions since — but this specific line came through unchanged.
   *Possible solution:* unchanged from last time — drop `* deltaTime` from both angle calculations; tune feel via `m_sensitivity` alone.

2. **The camera's hand-built world matrix only strips its own scale, not its parent's.** `GetViewMatrix()` builds a scale-free `T*R` for the camera object itself, then multiplies in `m_owner->GetParent()->GetWorldTransform()` when there's a parent — but that call still includes the *parent's* scale in full. Parenting the camera under a non-uniformly-scaled object would still skew the view, which defeats the point of stripping scale in the first place — and it re-implements "how position+rotation combine into a matrix" a second time, separately from `GetLocalTransform()`.
   *Possible solution:* decide once whether cameras should ever inherit ancestor scale. If not, walk the parent chain building only position+rotation at every level (a `GetWorldPositionAndRotation()`-style helper); if scale-from-parents is fine, just call `m_owner->GetWorldTransform()` and accept it.

3. **Direct uniform-pushing around `Material` — still unaddressed** *(open three commits running)* — `RenderQueue::Draw` still sets `uModel`/`uView`/`uProjection` straight on the shader program, bypassing `Material::SetParam`/`Bind()` entirely.
   *Possible solution:* unchanged — give `Material` a `glm::mat4` overload for `SetParam`, and route every per-draw uniform through `Bind()`.

4. **Mouse "old" position still gets shifted in two places per frame** *(open two commits running)* — both inside `cursorPositionCallback` (on every raw GLFW event) and once more at the end of `Engine::Run()`, which can silently drop movement if more than one mouse-move event lands within a single frame.
   *Possible solution:* unchanged — do the shift in exactly one of those two places, not both.

## Commit log

| Date | Summary |
|---|---|
| 2026-08-21 | Initial commit — engine core: `Engine` singleton (GLFW window/context, game loop), `Application` interface, `InputManager`, `GraphicsAPI`, `ShaderProgram`, `Material`, `Mesh`, vendored GLFW 3.4 + GLEW, sample triangle-rendering `Game`. |
| 2026-09-10 | Scene/GameObject hierarchy + `TestObject` sample — added `GameObject` (parent/child ownership, `IsAlive()`/`MarkForDestroy()`, cascading `Update()`), `Scene` (object storage, templated `CreateObject<T>()`, `SetParent()` with cycle checking), and `TestObject` as the first real `GameObject` (own shader/material/mesh, WASD-driven `uOffset`, submits to `RenderQueue`). |
| 2026-09-10 | Transform hierarchy via GLM — `GameObject` gained position/rotation/scale plus `GetLocalTransform()`/`GetWorldTransfrom()`; `ShaderProgram`, `RenderQueue`, and `TestObject` rewired to build and submit a `uModel` matrix instead of the old `uOffset` uniform (also fixed the frame-rate-dependent movement flagged last time). |
| 2026-09-13 | Component system + camera + full MVP pipeline — added `Component`/`MeshComponent`/`CameraComponent`; `RenderQueue`/`Engine` now build real `uView`/`uProjection` matrices from the scene's main camera each frame; fixed the `GetWorldTransfrom()` typo and its missing-return bug; reworked `engine/CMakeLists.txt`'s GLM linking. |
| 2026-09-19 | Mouse look/movement, depth testing, real shader-compile checking — added mouse tracking to `InputManager`/`Engine`, `PlayerControllerComponent` (look + move, attached to the camera), `GraphicsAPI::Init()` enabling depth testing, and real compile/link error checking in `CreateShaderProgram`. `TestObject` is now a static cube. |
| 2026-09-24 | Quaternion rotation + two bug fixes — `GameObject` rotation switched from Euler `glm::vec3` to `glm::quat`; `PlayerControllerComponent` mouse-look and `CameraComponent::GetViewMatrix()` rewritten for quaternions; fixed the `CreateIndexBuffer` size bug (open since the first commit) and the `PlayerControllerConmponent` typo. |
