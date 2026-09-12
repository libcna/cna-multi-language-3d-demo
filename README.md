# CNA Cross-Language 3D Demo

This repository specifies one small 3D game whose C++ reference is being
integrated with CNA. Its C directory is a consumer of CNA's existing C ABI,
while the other language directories contain equivalent demonstrations. It is
a demonstration project, not a production game: the shared gameplay should
remain small, readable, and easy to port.

## Project goals

- Use the CNA library, the C++ reimplementation of the XNA 4.0 API.
- Use only the XNA 4.0 API surface for game-facing code. CNA-specific graphics
  extensions (`CNA.Ext`) are deliberately out of scope.
- Use EasyGL as the initial internal renderer on Linux desktop, with OpenGLES
  as the first equivalent renderer option. Keep rendering behind a small
  backend interface so other equivalent renderers can be added later.
- Keep every implementation visually and behaviorally equivalent.
- Demonstrate CNA's C ABI and ports to C#, Java, TypeScript, Python, Rust, Go,
  Swift, Ruby, and Common Lisp.
- License all project code under the MIT License.

## Game concept

**CNA Starfield Courier** is a compact third-person 3D game. The player pilots
a small courier drone through a bounded asteroid arena, collects three energy
cells, avoids slow-moving hazards, and reaches the extraction ring. A run has
one scene, one camera, a tiny HUD, deterministic movement, collision checks,
and a win/lose/restart loop. No networking, procedural world generation, or
content pipeline beyond the minimum demonstration assets is planned.

The reference behavior, input mapping, coordinate conventions, asset list,
and cross-language conformance rules are recorded in `plan.md` and
`shared/game-contract.md`.

## Repository layout

Each language directory contains, or is being prepared to contain, the same
demo and a short build/run guide:

`cpp`, `c`, `cs`, `java`, `ts`, `python`, `rust`, `go`, `swift`, `ruby`, and
`common-lisp`.

The C++ version is the behavioral reference. With `STARFIELD_ENABLE_CNA=ON`,
`cpp/starfield_cna` is an XNA-shaped `Microsoft.Xna.Framework.Game` subclass
linked to CNA's `cna_runtime`; it clears the CNA graphics device through the
public XNA API. The `c` directory contains a pure-C CNA game consumer using
`CNA_GameCallbacks`, `cna_game_run_one_frame`, and `cna_game_clear`; it does
not wrap `starfield_core` or introduce a second game ABI. The other language
directories currently provide small headless ports.

## Development status

The repository contains the renderer-independent C++ reference, an optional
CNA-backed XNA game shell, an optional pure-C CNA game consumer, and headless
ports for C#, Java, TypeScript, Python, Rust, Go, Swift, Ruby, and Common Lisp.
The ports use the six scenarios in `shared/game-contract.md` so behavior can
be compared before their own binding and renderer integration.

## Binding status

The following statements are deliberately precise:

- The optional CMake integration consumes the real CNA tree from `../cna`.
  `starfield_cna` uses CNA's C++ XNA-compatible `Game` and
  `GraphicsDevice::Clear`; `cna_starfield_c` uses the public C lifecycle and
  graphics functions. The ABI smoke test independently checks the linked
  `cna_c_api` version.
- The optional C# probe consumes the real `CNA.XnaCompat` project from
  `../cna-cs` and compiles an XNA-shaped `Microsoft.Xna.Framework.Vector3`.
  The headless C# gameplay runner is still independent of that binding.
- Java, TypeScript, Python, Rust, Go, Swift, Ruby, and Common Lisp currently
  have contract-compatible headless ports only. They do not yet import or
  link their corresponding CNA binding repositories.
- Therefore matching snapshots prove only gameplay-contract conformance; they
  do not prove that a binding or a renderer is being used.

The optional `STARFIELD_ENABLE_CNA` CMake configuration consumes the local CNA
tree from `../cna`, builds the real `cna_runtime` and `cna_c_api` targets, and
provides both CNA game runners plus a direct `<CNA/C/cna.h>` ABI-version smoke
test. The C# project has a separate
`UseCnaCs=true` compile probe for the local `cna-cs` `CNA.XnaCompat` facade;
the probe proves the binding compiles, while the headless gameplay remains
independent of it.

## Headless conformance

The native reference is built with CMake. Available ports can then be checked
with their language tools, for example:

```sh
cmake -S . -B build
cmake --build build
build/cpp/starfield_cpp win
javac java/Starfield.java && java -cp java Starfield win
dotnet run --project cs/Starfield.csproj -- win
python3 python/starfield.py win
cargo run --manifest-path rust/Cargo.toml -- win
sbcl --script common-lisp/starfield.lisp win
```

To build and run the CNA-backed C++ and C demos (requires `../cna`):

```sh
cmake -S . -B build/cna-demo -DSTARFIELD_ENABLE_CNA=ON
cmake --build build/cna-demo --target starfield_cna cna_starfield_c cna_c_api_consumer_smoke
build/cna-demo/cpp/starfield_cna --frames 3
build/cna-demo/c/cna_starfield_c --frames 3
build/cna-demo/c/cna_c_api_consumer_smoke
```

To compile the C# XNA-shaped binding probe (requires `../cna-cs`):

```sh
dotnet build cs/Starfield.csproj -c Release \
  -p:UseCnaCs=true -p:CnaCsRoot="$PWD/../cna-cs"
```

Every headless runner prints `state mask x z elapsed score`; the expected
values and float tolerance are defined in the shared contract. Toolchains that
are not installed can use the corresponding source and README without
changing the simulation contract. A passing headless scenario is not evidence
that a language binding or a renderer is in use.

## License

MIT. See `LICENSE`.