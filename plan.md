# CNA Starfield Courier — Implementation Plan

## 1. Definition of done

The project is complete when all eleven language directories contain a small,
buildable implementation of the same game, the C ABI is documented and
usable, and the implementations pass the same deterministic conformance
scenarios. The Linux desktop reference renderer is EasyGL; OpenGLES is an
equivalent backend option and must not change game behavior.

Every public game-facing operation must map to the XNA 4.0 API model. CNA
graphics extensions (`CNA.Ext`) must not be used. Renderer-specific details
must remain below the backend boundary.

## 2. Shared game specification

1. Define a fixed 1280x720 reference presentation and a 3D right-handed world.
2. Define the scene: an arena, courier drone, three energy cells, hazards, and
   an extraction ring. Use generated primitives or tiny openly licensed assets.
3. Define controls: keyboard movement, yaw, boost, restart, and quit. Document
   key names and frame-independent behavior.
4. Define the state machine: `Title`, `Playing`, `Won`, and `Lost`.
5. Define deterministic rules: fixed timestep option, seeded hazard motion,
   collision radii, score, timer, and restart semantics.
6. Define the camera, lighting, colors, HUD text, sound policy, and asset names.
7. Create golden scenarios for startup, collection, hazard collision, win,
   loss, restart, and renderer-independent simulation.

## 3. Architecture and interfaces

1. Keep a small platform-neutral core for state, input, simulation, collision,
   and scoring.
2. Define an XNA-shaped rendering contract using only concepts available in
   XNA 4.0: game loop, `GameTime`, `Matrix`, `Vector3`, `Model`, effects,
   textures, vertex/index buffers, sprite batch, and input services.
3. Define a narrow renderer service and implement EasyGL first. Add OpenGLES
   as a second backend only after the EasyGL reference is stable.
4. Define the C ABI with opaque handles, explicit ownership, fixed-width types,
   error codes, UTF-8 strings, and a version query. Document ABI size and
   alignment assumptions.
5. Keep asset loading, timing, and window integration replaceable; do not let
   any language binding leak renderer objects into the game core.

## 4. Ordered implementation milestones

### 4.1 C++ reference

Implement the specification in `cpp` using CNA and EasyGL. Add the game loop,
simulation, XNA-compatible math and rendering calls, HUD, input, generated
assets, and a deterministic headless mode. Add unit tests for math and game
state plus a short manual Linux desktop run guide.

### 4.2 C ABI

Implement `c` as the stable façade over the C++ reference. Provide headers,
versioned symbols, lifecycle functions, input/update/render functions,
snapshot accessors, and error reporting. Add a C smoke test and ABI example.

### 4.3 C#

Port the game contract to `cs`, preserving XNA 4.0 naming and behavior. Use
the C ABI only where appropriate, document runtime prerequisites, and compare
the golden scenarios with the reference.

### 4.4 Java

Implement `java` with a small native bridge or documented ABI adapter. Keep
the public game model equivalent and add a deterministic test runner.

### 4.5 TypeScript

Implement `ts` with a typed ABI wrapper and a desktop runner. Keep rendering
behind the same backend contract rather than introducing browser-only rules.

### 4.6 Python

Implement `python` with a thin ctypes/cffi-style wrapper, a headless test
runner, and the smallest possible desktop launcher.

### 4.7 Go

Implement `go` with explicit ownership around the C ABI and a deterministic
conformance command.

### 4.8 Ruby

Implement `ruby` with a minimal FFI adapter, matching the shared lifecycle and
input contract.

### 4.9 Swift

Implement `swift` with a C interop module and Linux desktop instructions.
Verify value types and UTF-8/error handling at the boundary.

### 4.10 Common Lisp

Implement `common-lisp` with a portable foreign-function interface, keeping
the renderer optional for headless conformance runs.

## 5. Conformance and quality gates

1. Run formatting, compilation, and focused tests per language.
2. Run all golden scenarios against every implementation in headless mode.
3. Compare serialized state snapshots with tolerances documented for floats.
4. Run a Linux EasyGL visual smoke test and an OpenGLES smoke test when the
   second backend is available.
5. Check that no game-facing source includes CNA.Ext APIs.
6. Check memory ownership, ABI versioning, error paths, restart behavior, and
   clean shutdown.
7. Keep examples short, build instructions reproducible, and each milestone
   reviewable in a separate commit on `develop`.

## 6. Suggested delivery sequence

1. Establish the reference specification and repository conventions.
2. Finish the C++/EasyGL reference and headless tests.
3. Freeze and document the C ABI.
4. Port languages in this order: C#, Java, TypeScript, Python, Go, Ruby,
   Swift, and Common Lisp.
5. Add OpenGLES parity and final cross-language reports.
6. Tag a small MIT-licensed demonstration release from `main` after review.