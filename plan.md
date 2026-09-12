# CNA Starfield Courier — Graphical Delivery Ledger

This ledger replaces the failed headless prototype. A language is **DONE** only
when it launches a CNA game/window, enters the CNA loop, accepts real input, and
renders the playable 3D scene. Numeric simulation tests are secondary evidence.
All implementation commits belong on `develop`; `main` remains the clean
repository foundation.

## Status markers

`EMPTY` = reserved directory with no implementation, `WIP` = actively
investigated or implemented, `BLOCKED` = an evidenced CNA/binding limitation,
`DONE` = runtime-verified graphical game.

## Task ledger

1. **Repository repair — DONE for C++ milestone**
   - Keep `main` at the clean foundation commit and move implementation work to `develop`.
   - Ignore IDE files, build trees, binaries, compiler intermediates, caches, and language outputs.
   - Make coherent milestone commits; never commit generated output.
2. **Actual CNA API investigation — DONE**
   - Read the real public Game, GraphicsDevice, math, input, vertex/index, effect, and lifecycle declarations.
   - Confirm EasyGL and OpenGLES selection/build procedures from CNA sources and examples.
   - Prohibit CNAEXT, direct OpenGL calls, and invented APIs in game-facing code.
3. **Binding inventory — WIP**
   - Inspect each sibling binding, template, example, native bridge, and test before selecting an adapter.
   - Record exact missing capabilities as blockers; never claim a headless program is a port.
4. **Exact shared game specification — DONE**
   - Define deterministic arena geometry, colors, procedural meshes, object positions, hazards, collisions, timing, score, win/loss/restart, and controls.
   - Define a third-person perspective camera, FOV, clipping planes, depth, lighting, and reference presentation.
   - Define optional scripted startup/frame capture mode without replacing interactive play.
5. **C++ graphical reference implementation — DONE**
   - Replace the rejected frontend/core split with one authoritative `StarfieldGame : Microsoft::Xna::Framework::Game`.
   - Put reset, keyboard input, simulation, collision, score, terminal state, camera, and drawing in that class's real XNA lifecycle.
   - Render the live player, collectible, and hazard fields directly; do not introduce a snapshot transport or a second executable.
   - **Audio feedback — DONE:** four original PCM16 WAV cues load through XNA `SoundEffect::FromStream`; collection, loss, win, and restart use XNA `SoundEffect::Play` without CNAEXT.
   - **Background music — DONE:** the CC0/Public Domain `Outer Space Loop` is documented with source and checksum, and loops quietly through XNA `Song`/`MediaPlayer` without CNAEXT.
   - **Multi-sector campaign — DONE:** three sequential sectors have distinct palettes, landmarks, layouts, gates, and authoritative horizontal, vertical, and orbital hazards; gameplay and both renderer profiles validate every sector.
6. **EasyGL validation — DONE**
   - Built and ran the `OPENGL33` EasyGL implementation with a real 1280x720 X11 client window and with SDL's offscreen video driver.
   - Real keyboard events verified turn, heading-relative movement, collection, loss, win, restart, and Escape; the captured scene and heading-following camera were visually inspected.
7. **OpenGLES validation — DONE**
   - Ran the unchanged C++ game with the `OPENGLES3` EasyGL profile in real-window and offscreen configurations.
   - The final deterministic `OPENGL33` and `OPENGLES3` captures were byte-identical.
8. **Pure C graphical CNA implementation — EMPTY**
   - Use only the real public CNA C ABI; prove window/loop/input/3D rendering or document the exact ABI blocker.
9. **C# graphical implementation — EMPTY**
   - Use the real CNA C# binding or a minimal real-C-ABI FFI; no independent simulator milestone.
10. **Java graphical implementation — EMPTY**
    - Use the real CNA Java binding/native bridge and validate runtime loading and rendering.
11. **TypeScript graphical implementation — EMPTY**
    - Use the actual CNA TypeScript/native route available in the sibling repository; document unavailable desktop capabilities.
12. **Python graphical implementation — EMPTY**
    - Use the actual CNA Python binding or minimal C-ABI FFI and render through CNA.
13. **Rust graphical implementation — EMPTY**
    - Use the actual CNA Rust binding or minimal C-ABI FFI and validate ownership/lifecycle.
14. **Go graphical implementation — EMPTY**
    - Use the actual CNA Go binding or minimal C-ABI FFI and validate native loading.
15. **Swift graphical implementation — EMPTY**
    - Use the actual CNA Swift/C interop path and validate Linux runtime behavior.
16. **Ruby graphical implementation — EMPTY**
    - Use the actual CNA Ruby FFI path and validate window and render lifecycle.
17. **Common Lisp graphical implementation — EMPTY**
    - Use the actual CNA Lisp FFI path and validate interactive rendering.
18. **Visual/behavior parity testing — WIP**
   - Compare deterministic screenshots/frames and gameplay checkpoints against the C++ reference.
   - Require perspective, depth, object placement, colors, motion, collisions, and controls to match closely.
   - The rebuilt C++ application retains only finite graphical smoke/capture support that still enters `Game::Run()`; the old headless scenario protocol is removed.
19. **Build documentation — WIP**
    - Document verified prerequisites and exact build/run commands per language and renderer.
    - Keep README status limited to evidence-backed `graphical / verified`, `blocked`, or `not started`.
20. **Final cleanup and verification — DONE for C++ milestone**
    - Generated artifacts remain ignored, source audits are clean, both renderers are runtime-verified, and all C++ tests pass.

## Required implementation order

C++ → C → C# → Java → TypeScript → Python → Rust → Go → Swift → Ruby →
Common Lisp. Do not generate placeholder ports ahead of the active milestone.

## C++ architecture audit (2026-09-12, before repair)

- Starting commit: `f8aff38` (`cpp: complete and validate graphical reference`); the worktree was clean.
- The apparent CNA game in `cpp/src/cna_game.cpp` is only a shell. It owns a separate `starfield::Game simulation_`, converts `Keyboard::GetState()` into a custom `starfield::Input`, advances the independent simulation, copies a `starfield::Snapshot`, and draws that copy.
- `cpp/include/starfield.hpp` declares the competing generic `starfield::Game`, custom `Input`, custom `Snapshot`, generic `Renderer`, and `run_scenario()`. `cpp/src/starfield.cpp` contains the authoritative rules, while `cpp/src/main.cpp` exposes them as the default headless `starfield_cpp` executable.
- `cpp/CMakeLists.txt` makes `starfield_core` and the headless executable unconditional, but makes the actual CNA application an optional `starfield_cna` target. This states the inverse of the required architecture.
- `shared/game-contract.md` explicitly calls the graphical shell non-authoritative and defines serialized headless snapshots as the conformance protocol. That contract must be rewritten around the real graphical game.
- The existing tests validate the substitute `starfield::Game`; they do not instantiate or test the state owned by the XNA game class. The graphical scenario tests merely ask the wrapper to render snapshots precomputed by `run_scenario()`.
- The CNA checkout inspected at sibling commit `1b3151f2f` confirms the exact lifecycle and inheritance model: `Microsoft::Xna::Framework::Game` exposes virtual `Initialize`, `LoadContent`, `Update(GameTime&)`, and `Draw(const GameTime&)`, and `Game::Run()` performs device creation, lifecycle initialization, event polling, timed updates, drawing, and presentation.
- CNA's headers mark extensions with `CNAEXT`. The APIs required here are on the non-extension XNA-compatible surface: `GraphicsDeviceManager`, `Keyboard::GetState`, `Keys`, `BasicEffect`, `Matrix`, `Vector3`, `DepthStencilState`, `RasterizerState`, `RenderTarget2D`, and typed `GraphicsDevice::DrawUserPrimitives`.
- `Game::RunOneFrame()` is declared on CNA's non-`CNAEXT` compatibility surface and is used by CNA's lifecycle tests, but it is a finite-frame harness method rather than the correct normal application entry. The rebuilt application will not call it: playable and automated graphical runs will both enter ordinary `Game::Run()` and exit from the game lifecycle.
- Renderer choice is a CNA build/runtime configuration concern. A CNA multi-renderer build accepts `CNA_GRAPHICS_RENDERERS="OPENGL33;OPENGLES3"`, with `CNA_GRAPHICS_RENDERER` selecting the default or environment-selected compiled renderer. The game source must not inspect that selection.

## C++ repair boundary

- Delete the custom framework files and replace them with a directly owned XNA game implementation and narrowly named domain data (`Player`, `Hazard`, `Collectible`, and run state).
- Build `starfield_cpp` as the sole C++ application and require/link CNA for the C++ project by default. A small library target may package the actual `StarfieldGame` class for its executable and tests; it must not be independently runnable or become a second simulation.
- Tests may use a friend test-access class to invoke the exact private gameplay step owned by `StarfieldGame`. They must not duplicate the rules, create a second game, or serialize snapshots. A graphical smoke test must run the application through `Game::Run()` and validate the real rendered frame.
- C and every later language remain out of scope until this corrected C++ milestone is complete.

## C++ verification evidence after repair (2026-09-12)

- `cpp/include/StarfieldGame.hpp` declares the only game class, and compile-time tests prove it is `final` and derives from `Microsoft::Xna::Framework::Game`.
- `cpp/src/StarfieldGame.cpp` owns reset, resource creation, CNA keyboard reads, simulation, player movement/heading, chase camera, sector loading, up to three active hazard objects, collectibles, collision, timer, score, win/loss/restart, world rendering, and HUD rendering. `Draw` reads the same object fields collision uses; there is no snapshot copy.
- The default CMake project requires the sibling CNA source and builds `starfield_cpp` as the CNA application. The old `starfield_core`, headless `starfield_cpp`, optional `starfield_cna`, scenarios, custom `Game`, `Input`, `Snapshot`, and `Renderer` are gone.
- `cpp_gameplay` tests the actual `StarfieldGame` fields and private gameplay step. It passed initial state, turning, heading-relative movement, heading-following camera, authoritative collision, a safe continuous 60 Hz first-sector route, horizontal/vertical/orbital enemies, all three transitions, the `800 → 1600 → 2900` score progression, final win, terminal freeze, 60 Hz timeout, and full campaign restart.
- Six graphical tests enter ordinary `Game::Run()` and validate all three 1280x720 sector frames under both OpenGL33 and OpenGLES3. Together with `cpp_gameplay`, final CTest result is 7/7 passed.
- OpenGL33 initialized as EasyGL/OpenGL 4.5 offscreen and OpenGL 4.6 in the X11 window. The 1280x720 capture was visually inspected. Real `D` then `W` input produced heading `1.30899`, position `(1.60987,-0.431371)`, and a correspondingly rotated chase view; `R` restored position and heading to zero; Escape left `Game::Run()`.
- OpenGLES3 initialized as EasyGL/OpenGL ES 3.2 both offscreen and in an X11 run. Every validated sector capture is byte-identical to OpenGL33. Sector SHA-256 values are `0473210d005b2f955d936746f4f2ae7f64015af24d2e854cb54309a6851ef386`, `d5ea110d60c39ed44c2b2e1982b24f151400a963e57a5d59aa15912145e51573`, and `2669a1362ab5ee8d6a154183ae5c3e437408d581f3f4eb57828d49d1a66d3e55`.
- The final production-source audit found no `CNAEXT`/`cnaext`, `RunOneFrame`, CNA-specific namespace call, direct GL/GLES/EGL/SDL/Vulkan/DirectX include or call, renderer branch, custom generic `Game`, custom `Renderer`, custom `Input`, custom `Snapshot`, `run_scenario`, `starfield_core`, or `starfield_cna`.
- No CNA defect or missing capability was encountered. No CNA-specific API remains in production game code; the finite capture option also exits from `Draw` while the executable remains inside normal `Game::Run()`.
- A post-milestone playtest exposed poor terminal feedback: `Won` correctly froze gameplay but looked like a hang. The live game now retains presentation animation and displays an explicit, prominent win/loss panel with restart instructions. The procedural scene and HUD were also rebuilt with shaded composite objects and readable text.
- The initially evaluated NOX recordings were rejected after playtesting and removed. Four clean, original synthesized cues now cover collection, loss, win, and restart through standard XNA `SoundEffect` APIs.
- Quiet background music uses the unmodified CC0/Public Domain `Outer Space Loop` by wipics. Standard XNA `Song::FromUri` and `MediaPlayer` load, play, and repeat it; its source, license, and checksum are recorded beside the asset.
- C and all later language directories are `EMPTY` placeholders after removal
  of the invalid prototypes; no replacement port was started.
