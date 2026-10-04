# CNA Starfield Courier — three-language acceptance contract

This repository contains three implementations of one real, playable 3D game:
C++ through CNA, C through CNA's C binding, and one C# source through CNA.NET
and FNA. Graphics are part of conformance; a detached numerical simulator or a
window that only clears the framebuffer is not a port.

## Authority hierarchy

1. [game-spec.md](game-spec.md) defines the shared behavior and presentation.
2. [`../cpp/`](../cpp/) is the CNA/XNA C++ reference.
3. [`../c/`](../c/) is the direct public-C-ABI implementation.
4. [`../cs/`](../cs/) is the shared XNA 4.0 C# implementation. Its project
   reference may select CNA.NET or FNA, but `StarfieldGame.cs` remains common.

No custom cross-language game engine, renderer, snapshot transport, or
game-specific ABI sits between these implementations and CNA/FNA.

## Required application behavior

Each implementation must:

- enter the real CNA/XNA lifecycle and create the game window;
- receive keyboard input from the framework API;
- keep the live player, sector, collectible, hazard, timer, score, camera,
  terminal state, and restart state inside that lifecycle;
- render perspective 3D geometry for every required live object;
- render the HUD/terminal presentation and play the shared audio;
- use one authoritative position for gameplay, collision, camera, and drawing;
- be playable as its normal execution mode.

The C++ and C# game-facing graphics code targets XNA 4.0-style APIs. It must
not call OpenGL, Vulkan, DirectX, SDL rendering, EasyGL internals, or a custom
renderer. The C implementation uses the corresponding public CNA C ABI.
Renderer selection remains a runtime/build concern.

## C# differential requirement

`cs/StarfieldGame.cs` must compile unchanged against both CNA.NET's
`CNA.XnaCompat` and an XNA-compatible FNA assembly. Engine selection belongs
only in MSBuild references and launch configuration. A compatibility result is
not valid if game logic is forked with conditional compilation or copied into
separate CNA/FNA sources.

## Behavioral and visual parity

All three language ports implement the observable contract from
`game-spec.md`: world dimensions, sector layouts, movement and turn rates,
boost, collectible and hazard rules, gate progression, score, timer, chase
camera, procedural `BasicEffect` scene, HUD semantics, controls, audio, win,
loss, and restart.

Source organization and primitive mesh detail may be idiomatic to the binding.
The conformance requirement is the specified scene and behavior, not
byte-identical rasterization across different language runtimes or GPU
backends.

## Validation

A completed implementation requires both:

- deterministic behavior assertions covering movement, collision, sector
  progression, final victory, timeout/terminal state, and restart; and
- the normal graphical executable rendering a nonblank, semantically valid
  frame for all three sectors from the real runtime lifecycle.

C++ additionally compares its OpenGL 3.3 and OpenGL ES 3 captures. C and C#
validate framebuffer dimensions/content and required sector colors. The C#
checks must pass on both CNA.NET and FNA from the same source.

## Final repository status

| Language | Binding/runtime | Status |
| --- | --- | --- |
| C++ | CNA XNA-style API | **DONE** |
| C | CNA C ABI | **DONE** |
| C# | CNA.NET and FNA, shared source | **DONE** |

The three-language scope closed on 2026-10-04. Additional language ports are
not part of this repository's standing backlog.
