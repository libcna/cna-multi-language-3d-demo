# Building and validating Starfield Courier

## Current build scope

Only the C++ CNA/XNA reference is implemented. The root CMake project builds
that application and embeds CNA with `add_subdirectory`; CNA is not an optional
viewer around another executable. The other language directories contain only
`.gitkeep` and intentionally have no build commands yet.

Run every command below from the repository root.

## Prerequisites

The currently validated configuration is Linux with EasyGL's `OPENGL33` and
`OPENGLES3` profiles. It requires:

- CMake 3.20 or newer (the embedded CNA project sets the effective minimum);
- a C++23-capable compiler, such as GCC 12+ or Clang 15+;
- Ninja for the commands below, or another CMake generator if preferred;
- Git checkouts of CNA and its required sibling projects;
- an OpenGL 3.3 or OpenGL ES 3-capable driver for interactive use.

The default sibling layout is:

```text
parent/
├── cna-multi-language-3d-demo/
├── cna/
├── sharp-runtime/
├── easy-gl/
└── meta-gl/
```

CNA also requires its vendored dependencies. Initialize them in the CNA
checkout before the first build:

```sh
git -C ../cna submodule update --init --recursive
```

CNA builds SDL3, SDL3_image, and SDL3_mixer from those vendored sources by
default. Its optional video dependency remains in `AUTO` mode and is not
required by this game. See CNA's own build documentation when using a custom
dependency layout or system SDL packages.

## Configure, build, and play

Configure both validated renderer profiles into one out-of-tree Release build:

```sh
cmake -S . -B build/cna-game -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCNA_SOURCE_DIR=../cna \
  -DCNA_GRAPHICS_RENDERER=OPENGL33 \
  -DCNA_GRAPHICS_RENDERERS="OPENGL33;OPENGLES3"
```

`CNA_GRAPHICS_RENDERER` selects the default profile and
`CNA_GRAPHICS_RENDERERS` determines which profiles are compiled in. Build the
playable application and its tests:

```sh
cmake --build build/cna-game --target starfield_cpp starfield_cpp_tests
```

Launch either compiled renderer without changing game code:

```sh
CNA_GRAPHICS_RENDERER=OPENGL33 ./build/cna-game/cpp/starfield_cpp
CNA_GRAPHICS_RENDERER=OPENGLES3 ./build/cna-game/cpp/starfield_cpp
```

The `starfield_content` build dependency refreshes
`build/cna-game/cpp/Content` automatically, including the audio files. Run the
binary from another working directory only if its adjacent `Content` directory
remains available.

## Automated validation

Run the complete C++ validation suite:

```sh
ctest --test-dir build/cna-game -L cpp --output-on-failure
```

The suite contains seven tests:

- one deterministic gameplay test against the real `StarfieldGame` state;
- three actual `Game::Run()` frame tests for sectors 1–3 with `OPENGL33`;
- three actual `Game::Run()` frame tests for sectors 1–3 with `OPENGLES3`.

CTest supplies the offscreen video and dummy audio environment for graphical
tests. Individual groups can be selected with `-L gameplay`, `-L opengl33`, or
`-L opengles3`.

## Capture and renderer parity

Capture the deterministic initial frame of every sector from the real game
lifecycle:

```sh
mkdir -p build/cna-game/captures
for renderer in OPENGL33 OPENGLES3; do
  for sector in 1 2 3; do
    SDL_VIDEODRIVER=offscreen \
    SDL_AUDIODRIVER=dummy \
    CNA_GRAPHICS_RENDERER="$renderer" \
      ./build/cna-game/cpp/starfield_cpp \
        --smoke-frames 1 \
        --validate-frame \
        --start-sector "$sector" \
        --screenshot "build/cna-game/captures/${renderer}-sector${sector}.ppm"
  done
done
```

Each successful run reports `frame_valid=true` and writes a 1280×720 PPM read
back from CNA's render target. Compare renderer output and calculate checksums:

```sh
for sector in 1 2 3; do
  cmp "build/cna-game/captures/OPENGL33-sector${sector}.ppm" \
      "build/cna-game/captures/OPENGLES3-sector${sector}.ppm"
done
sha256sum build/cna-game/captures/*.ppm
```

The reference captures recorded on 2026-09-12 after the HUD and sector-banner
changes were byte-identical between renderers:

| Sector | SHA-256 for both renderer captures |
| --- | --- |
| 1 — Starport | `b27fa0afd06276c7e834063a6c5f60dc1bcd9adcce65213f7805b451c27812ef` |
| 2 — Ion Basin | `4194f8122c62c60f5d1a3a1de50b196581a6e158f60a8fe3ee86188ead0ba9f3` |
| 3 — Solar Forge | `750f61951f95bc58ef3c90f247d4d23aef37f76507c7b7e8e933bbdf7ad0ed9d` |

These hashes record the verified Mesa environment and current game revision.
Other conformant drivers may rasterize small edge details differently, so a
cross-machine mismatch requires visual investigation rather than automatic
failure. Equality between both renderer profiles on the same machine remains
the local parity check.

## Build-output policy

Use an out-of-tree directory under `build/` or named `build-*`; both forms are
ignored by Git. Do not commit executables, CMake caches, copied content, capture
PPMs, or dependency build products. The PNG shown in the main README is the
intentional documentation screenshot and is the only committed capture.
