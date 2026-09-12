# CNA Cross-Language 3D Demo

This repository specifies one small 3D game implemented consistently in C++
with CNA and then exposed through a C ABI for equivalent implementations in
other languages. It is a demonstration project, not a production game: the
shared gameplay should remain small, readable, and easy to port.

## Project goals

- Use the CNA library, the C++ reimplementation of the XNA 4.0 API.
- Use only the XNA 4.0 API surface for game-facing code. CNA-specific graphics
  extensions (`CNA.Ext`) are deliberately out of scope.
- Use EasyGL as the initial internal renderer on Linux desktop, with OpenGLES
  as the first equivalent renderer option. Keep rendering behind a small
  backend interface so other equivalent renderers can be added later.
- Keep every implementation visually and behaviorally equivalent.
- Demonstrate a C ABI binding and ports to C#, Java, TypeScript, Python, Rust,
  Go, Swift, Ruby, and Common Lisp.
- License all project code under the MIT License.

## Game concept

**CNA Starfield Courier** is a compact third-person 3D game. The player pilots
a small courier drone through a bounded asteroid arena, collects three energy
cells, avoids slow-moving hazards, and reaches the extraction ring. A run has
one scene, one camera, a tiny HUD, deterministic movement, collision checks,
and a win/lose/restart loop. No networking, procedural world generation, or
content pipeline beyond the minimum demonstration assets is planned.

The reference behavior, input mapping, coordinate conventions, asset list,
and cross-language conformance rules will be recorded in `plan.md` before
implementation begins.

## Repository layout

Each language directory will eventually contain the same demo and a short
build/run guide:

`cpp`, `c`, `cs`, `java`, `ts`, `python`, `rust`, `go`, `swift`, `ruby`, and
`common-lisp`.

The C++ version is the behavioral reference. The C version is the ABI layer;
the remaining implementations consume that ABI or mirror its explicitly
documented contract where a native binding is more appropriate.

## Development status

The repository currently contains the initial documentation and empty language
directories. Implementation work follows the ordered milestones in `plan.md`.

## License

MIT. See `LICENSE`.