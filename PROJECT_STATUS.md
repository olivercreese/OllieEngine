# OllieEngine — Project Status

*Last updated: 10 September 2026 — committed and pushed as "Add transform hierarchy (position/rotation/scale) via GLM matrices".*

This file is meant to be the fast-context doc for OllieEngine: what the engine currently does, how it's put together, and what was just changed — so a new chat (or a future you) can pick up the project without re-reading the whole codebase. Update it each time you commit: move today's "Latest Work" into the Commit Log, replace it with the new changes, and refresh the architecture snapshot if modules were added or restructured.

> **Note for Claude (or anyone helping with this project):** read this file first before answering questions about OllieEngine. It has the architecture, the most recent commit's changes, and where things stand — use it as the starting context instead of re-deriving it from scratch each session.

Each commit gets two kinds of questions below: **learning questions**, aimed at building up Ollie's understanding of C++ / engine programming / 3D graphics through the code just written (deliberately open — the point is to work them out, not read an answer); and **harder review questions**, which behave more like a code review and come with a possible solution attached, for the cases worth just knowing the answer to.

> **Question-writing preferences (Ollie's feedback, 10 Sept 2026) — apply to every future set:**
> - No OpenGL/graphics-API history or "deprecated feature" trivia (e.g. core vs. compatibility profile). Skip that entirely, not useful.
> - Mix in more questions about C++ itself — syntax/semantics, memory management (ownership, lifetimes, RAII, smart vs. raw pointers) — and abstract engine-architecture design, not just OpenGL/graphics mechanics.
> - Don't ask Ollie to reason about engine features that don't exist yet (e.g. "what would you need to add for X" when X isn't implemented). Note it as a roadmap/TODO item instead of turning it into a question.
> - Close each learning-question set with a small hands-on programming task rather than another conceptual question.

## What OllieEngine is

A small custom C++ game engine, built from scratch on top of GLFW (windowing/input), GLEW (OpenGL 3.3 core function loading), and GLM (vector/matrix math), using CMake. It's a learning/hobby engine project — currently a single "engine" static-ish library (`engine/`) consumed by a sample game executable (`source/`).

## Architecture snapshot

**`engine/source/`** — the engine library, namespace `eng`
- `Engine` (singleton) — owns the GLFW window/context, the main loop with delta time, the `InputManager`, and the `GraphicsAPI`. Also owns/drives the active `Scene` implicitly through the `Application`.
- `Application` — interface (`Init` / `Update` / `Destroy`) that game code implements; the engine drives it each frame.
- `input/InputManager` — polls keyboard state via GLFW callbacks.
- `graphics/GraphicsAPI` — wraps shader compilation and buffer creation.
- `graphics/ShaderProgram` — compiled shader program with cached uniform locations. `SetUniform` now has float, float-pair, **and `glm::mat4`** overloads.
- `render/Material` — binds a `ShaderProgram` and holds float/float2 uniform params.
- `render/Mesh` — VAO/VBO/EBO wrapper driven by a data-defined `VertexLayout`.
- `render/RenderQueue` — collects `RenderCommand`s (material + mesh + **modelMatrix** `(new)`) submitted during `Update`; `Draw()` now sets the command's `modelMatrix` as the shader's `uModel` uniform right before binding/drawing each mesh.
- `scene/GameObject` — base class for anything living in the scene: name, parent pointer, owned children, `IsAlive()` / `MarkForDestroy()` lifecycle, a virtual `Update()` that cascades to children and prunes dead ones, **and now position/rotation/scale (`glm::vec3`) plus `GetLocalTransform()` (builds a model matrix from them) and `GetWorldTransfrom()` (recursively composed through the parent chain) `(new)`**.
- `scene/Scene` — owns the root set of `GameObject`s. `CreateObject(name, parent)` makes a plain `GameObject`; the templated `CreateObject<T>(name, parent)` makes any `GameObject` subclass. `SetParent()` handles reparenting, including a cycle check.

**`source/`** — the sample game executable
- `main.cpp` — creates the `Game`, initializes the `Engine` at 1280×720, runs the loop.
- `Game` — implements `Application`; owns one `eng::Scene` (`m_scene`).
- `TestObject` **(reworked)** — a `GameObject` subclass: builds an inline shader (position + vertex color, now taking a `uniform mat4 uModel` instead of the old `uOffset`), a colored quad `Mesh`, moves itself with WASD by editing its inherited position (properly scaled by `deltaTime`), and submits `GetWorldTransfrom()` as its `RenderCommand`'s `modelMatrix` each frame.

**Third-party** (`engine/thirdparty/`): GLFW 3.4, GLEW, and **GLM 1.0.1 `(new)`** — all vendored and pulled in via `include_directories` in the root `CMakeLists.txt`.

**Build**: root `CMakeLists.txt` builds the `OllieEngine` executable from `source/*`, pulls in `engine/` as a subdirectory, and links the `Engine` library.

## Latest work (committed 10 Sept 2026)

Transform hierarchy via GLM matrices, replacing the ad-hoc `uOffset` approach:

- Vendored GLM 1.0.1 under `engine/thirdparty/`; wired into the build via `include_directories` in the root `CMakeLists.txt`.
- `GameObject` now owns position/rotation/scale (`glm::vec3`), plus `GetLocalTransform()` (builds a model matrix from them) and `GetWorldTransfrom()` (recursively composes through the parent chain).
- `ShaderProgram::SetUniform` gained a `glm::mat4` overload (via `glm::value_ptr`).
- `RenderCommand` gained a `modelMatrix` field; `RenderQueue::Draw` now sets it as the shader's `uModel` uniform immediately before binding/drawing each mesh.
- `TestObject` rewritten: the vertex shader now takes `uniform mat4 uModel` instead of `uOffset`; WASD movement edits `GetPosition()`/`SetPosition()`, properly scaled by `deltaTime` this time (closes harder-question #3 from the last commit), and the object submits `GetWorldTransfrom()` as its `modelMatrix` each frame.

**Suggested commit message:**

```
Add transform hierarchy (position/rotation/scale) via GLM matrices

- Vendor GLM 1.0.1 for vector/matrix math
- Add position/rotation/scale to GameObject, plus GetLocalTransform()
  and GetWorldTransfrom() (recursively composed through the parent
  chain)
- Add a glm::mat4 overload to ShaderProgram::SetUniform
- Add a modelMatrix field to RenderCommand; RenderQueue::Draw sets it
  as the uModel uniform before drawing
- Rewire TestObject to move via GameObject's position (now scaled by
  deltaTime) and render through uModel instead of the old ad-hoc
  uOffset uniform
```

## Learning questions from this commit — C++, engine architecture & the new transform system

Grounded in the actual code from this commit (`GameObject`, `TestObject`, `RenderQueue`). Worth working through by hand rather than just reading an answer.

1. `GameObject::GetPosition()` returns `const glm::vec3&`, but `TestObject::Update` does `auto position = GetPosition();` and later mutates `position.x` / `position.y` before calling `SetPosition(position)`. Given how `auto` deduces from a reference-returning function, is `position` here a reference to the real stored value or an independent copy? Does the code still behave correctly either way, and why?
2. `GameObject` stores its children as `std::vector<std::unique_ptr<GameObject>>` but exposes the parent as a raw `GameObject* m_parent`. What ownership rule does that split represent (who owns whom), and what specifically could go wrong with `m_parent` if a child ever outlived, or got reparented away from, the object it points to?
3. `GetLocalTransform()` builds its matrix as `translate`, then three `rotate` calls, then `scale`, each one multiplying onto the previous result. Matrix multiplication isn't commutative — if you swapped the order so `scale` happened first and `translate` last, what would visibly change about how the quad moves and spins? Try it and see.
4. Now that every `GameObject` has a full position/rotation/scale and `GetWorldTransfrom()` walks up through `m_parent`, what does parenting one `TestObject` under another actually buy you that plain sibling objects didn't have before? Think specifically about what moving the parent now does to the child.
5. **Programming task:** `GameObject` has `SetRotation()`/`GetRotation()` but nothing that changes rotation incrementally. Add a `RotateBy(const glm::vec3& delta)` method to `GameObject`, then wire up two new keys in `TestObject::Update` (e.g. `Q`/`E`) that spin the quad around the Z axis using it, scaled by `deltaTime`.

## A few harder questions (with possible solutions)

More "code review" than "learning exercise" — real gaps or bugs, each with one reasonable way to close it.

1. **`GetWorldTransfrom()` doesn't return anything when there's no parent.**
   ```cpp
   glm::mat4 GameObject::GetWorldTransfrom() const
   {
       if (m_parent)
       {
           return m_parent->GetWorldTransfrom() * GetLocalTransform();
       }
       else
       {
           GetLocalTransform();
       }
   }
   ```
   The `else` branch computes `GetLocalTransform()` and throws the result away — falling off the end of a non-`void` function, which is undefined behavior in C++, not a guaranteed zero or a guaranteed "it just returns the local transform anyway." It likely *appears* to work today (leftover value in the return register from the call just made), which is exactly what makes this class of bug dangerous — it can silently break on a different compiler, optimization level, or even just a rebuild.
   *Possible solution:* `return GetLocalTransform();` in the `else`, or drop the branch entirely: `return m_parent ? m_parent->GetWorldTransfrom() * GetLocalTransform() : GetLocalTransform();`.

2. **The model matrix uniform bypasses `Material`'s own parameter system.** `RenderQueue::Draw` reaches past `Material` and calls `command.material->GetShaderProgram()->SetUniform("uModel", ...)` directly — `Material::SetParam`/`Bind()` still only know about float and float2 params, so the model matrix isn't tracked as material state at all. Every future matrix uniform (view, projection) risks getting wired in with the same one-off special case instead of going through one consistent path.
   *Possible solution:* add a `SetParam(name, const glm::mat4&)` overload to `Material` (mirroring the float/float2 ones already there), store the model matrix on the material before `RenderQueue::Draw` runs, and let `Material::Bind()` push it like every other uniform.

3. **Index buffer still uploads the wrong type's byte size** *(carried over from last commit, not yet fixed)* — `GraphicsAPI::CreateIndexBuffer` still computes `indices.size() * sizeof(float)` for a `std::vector<uint32_t>`.
   *Possible solution:* unchanged from last time — `indices.size() * sizeof(uint32_t)` (or `sizeof(indices[0])`).

4. **Shader-compile failure still isn't checked** *(carried over from last commit, not yet fixed)* — `TestObject`'s constructor still calls `m_material.SetShaderProgram(shaderProgram)` with no null check on `CreateShaderProgram`'s result.
   *Possible solution:* unchanged from last time — check the returned `shared_ptr` for null and log + bail (or fall back to an "error" shader) before building the mesh.

5. **`GetWorldTransfrom()` recomputes the whole parent chain from scratch on every call.** Fine for the current shallow hierarchy, but it's called once per object per frame from `TestObject::Update`, and every call walks all the way to the root recomputing every ancestor's local transform — cost grows with both hierarchy depth and object count.
   *Possible solution:* cache the computed world matrix on each `GameObject` and only recompute it when a "dirty" flag is set (position/rotation/scale changed, or the parent's cached matrix changed) — the classic scene-graph dirty-flag pattern.

## Commit log

| Date | Summary |
|---|---|
| 2026-08-21 | Initial commit — engine core: `Engine` singleton (GLFW window/context, game loop), `Application` interface, `InputManager`, `GraphicsAPI`, `ShaderProgram`, `Material`, `Mesh`, vendored GLFW 3.4 + GLEW, sample triangle-rendering `Game`. |
| 2026-09-10 | Scene/GameObject hierarchy + `TestObject` sample — added `GameObject` (parent/child ownership, `IsAlive()`/`MarkForDestroy()`, cascading `Update()`), `Scene` (object storage, templated `CreateObject<T>()`, `SetParent()` with cycle checking), and `TestObject` as the first real `GameObject` (own shader/material/mesh, WASD-driven `uOffset`, submits to `RenderQueue`). |
| 2026-09-10 | Transform hierarchy via GLM — `GameObject` gained position/rotation/scale plus `GetLocalTransform()`/`GetWorldTransfrom()`; `ShaderProgram`, `RenderQueue`, and `TestObject` rewired to build and submit a `uModel` matrix instead of the old `uOffset` uniform (also fixes the frame-rate-dependent movement flagged last time). |
