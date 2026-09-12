# CNA Starfield Courier — Graphical Delivery Ledger

This ledger replaces the failed headless prototype. A language is **DONE** only
when it launches a CNA game/window, enters the CNA loop, accepts real input, and
renders the playable 3D scene. Numeric simulation tests are secondary evidence.
All implementation commits belong on `develop`; `main` remains the clean
repository foundation.

## Status markers

`TODO` = not started, `WIP` = actively investigated or implemented, `BLOCKED`
= an evidenced CNA/binding limitation, `DONE` = runtime-verified graphical game.

## Task ledger

1. **Repository repair — WIP**
   - Keep `main` at the clean foundation commit and move implementation work to `develop`.
   - Ignore IDE files, build trees, binaries, compiler intermediates, caches, and language outputs.
   - Make coherent milestone commits; never commit generated output.
2. **Actual CNA API investigation — WIP**
   - Read the real public Game, GraphicsDevice, math, input, vertex/index, effect, and lifecycle declarations.
   - Confirm EasyGL and OpenGLES selection/build procedures from CNA sources and examples.
   - Prohibit CNAEXT, direct OpenGL calls, and invented APIs in game-facing code.
3. **Binding inventory — WIP**
   - Inspect each sibling binding, template, example, native bridge, and test before selecting an adapter.
   - Record exact missing capabilities as blockers; never claim a headless program is a port.
4. **Exact shared game specification — WIP**
   - Define deterministic arena geometry, colors, procedural meshes, object positions, hazards, collisions, timing, score, win/loss/restart, and controls.
   - Define a third-person perspective camera, FOV, clipping planes, depth, lighting, and reference presentation.
   - Define optional scripted startup/frame capture mode without replacing interactive play.
5. **C++ graphical reference implementation — TODO**
   - Replace the Clear-only shell with the actual CNA Game, input, procedural 3D geometry, camera, depth, lighting, HUD/state, and gameplay.
   - Keep game-facing rendering strictly XNA 4.0 style and renderer-independent.
6. **EasyGL validation — TODO**
   - Build, launch a real window, capture/inspect a deterministic frame, and exercise collect, hazard loss, win, restart, and quit.
7. **OpenGLES validation — TODO**
   - Run the same C++ source with CNA’s actual OpenGLES renderer and verify equivalent gameplay and framing.
8. **Pure C graphical CNA implementation — TODO**
   - Use only the real public CNA C ABI; prove window/loop/input/3D rendering or document the exact ABI blocker.
9. **C# graphical implementation — TODO**
   - Use the real CNA C# binding or a minimal real-C-ABI FFI; no independent simulator milestone.
10. **Java graphical implementation — TODO**
    - Use the real CNA Java binding/native bridge and validate runtime loading and rendering.
11. **TypeScript graphical implementation — TODO**
    - Use the actual CNA TypeScript/native route available in the sibling repository; document unavailable desktop capabilities.
12. **Python graphical implementation — TODO**
    - Use the actual CNA Python binding or minimal C-ABI FFI and render through CNA.
13. **Rust graphical implementation — TODO**
    - Use the actual CNA Rust binding or minimal C-ABI FFI and validate ownership/lifecycle.
14. **Go graphical implementation — TODO**
    - Use the actual CNA Go binding or minimal C-ABI FFI and validate native loading.
15. **Swift graphical implementation — TODO**
    - Use the actual CNA Swift/C interop path and validate Linux runtime behavior.
16. **Ruby graphical implementation — TODO**
    - Use the actual CNA Ruby FFI path and validate window and render lifecycle.
17. **Common Lisp graphical implementation — TODO**
    - Use the actual CNA Lisp FFI path and validate interactive rendering.
18. **Visual/behavior parity testing — TODO**
    - Compare deterministic screenshots/frames and gameplay checkpoints against the C++ reference.
    - Require perspective, depth, object placement, colors, motion, collisions, and controls to match closely.
19. **Build documentation — TODO**
    - Document verified prerequisites and exact build/run commands per language and renderer.
    - Keep README status limited to evidence-backed `graphical / verified`, `blocked`, or `not started`.
20. **Final cleanup and verification — TODO**
    - Remove generated artifacts, verify ignore rules and clean worktree, audit for CNAEXT/direct renderer calls, and run the relevant tests.

## Required implementation order

C++ → C → C# → Java → TypeScript → Python → Rust → Go → Swift → Ruby →
Common Lisp. Do not generate placeholder ports ahead of the active milestone.