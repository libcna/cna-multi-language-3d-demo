# Building and validating Starfield Courier

Run the commands below from the repository root. Generated output belongs
under `build/`, `bin/`, or `obj/` and must not be committed.

## Prerequisites

The validated development configuration is Linux with:

- CMake 3.20 or newer and Ninja;
- a C++23 compiler and a C99 compiler;
- .NET SDK 8.0 or newer;
- sibling CNA, sharp-runtime, EasyGL, and meta-gl checkouts;
- a sibling CNA.NET (`cna-cs`) checkout for the C# CNA build;
- an OpenGL 3.3 or OpenGL ES 3-capable driver.

The expected source layout is:

```text
parent/
├── cna-multi-language-3d-demo/
├── cna/
├── cna-cs/
├── sharp-runtime/
├── easy-gl/
└── meta-gl/
```

Initialize CNA's vendored dependencies before its first build:

```sh
git -C ../cna submodule update --init --recursive
```

## C++ and C

Configure CNA with the C API and both validated renderer profiles. The demo's
root project enables the C API automatically:

```sh
cmake -S . -B build/cna-game -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCNA_SOURCE_DIR=../cna \
  -DCNA_GRAPHICS_RENDERER=OPENGL33 \
  -DCNA_GRAPHICS_RENDERERS="OPENGL33;OPENGLES3"
```

Build both applications and both gameplay test executables:

```sh
cmake --build build/cna-game \
  --target starfield_cpp starfield_cpp_tests starfield_c starfield_c_tests
```

Run interactively:

```sh
CNA_GRAPHICS_RENDERER=OPENGL33 ./build/cna-game/cpp/starfield_cpp
./build/cna-game/c/starfield_c
```

The CMake suite contains eleven native tests: seven for the C++ reference and
four for the C port. It includes deterministic gameplay and a real
`Game::Run()`/C-lifecycle frame from every sector. CNA contributors should use
the isolated GPU runner instead of the owner's desktop:

```sh
../cna/tools/platform/run_gpu_tests_private.sh \
  "$PWD/build/cna-game" -L '^(cpp|c)$' --output-on-failure -j4
```

## C# on CNA.NET

The application and gameplay test project each compile the same
`cs/StarfieldGame.cs`. Point the projects at the CNA.NET checkout:

```sh
CNA_CS_ROOT="$PWD/../cna-cs" dotnet build cs/StarfieldCourier.csproj \
  -c Release -p:Engine=CNA
CNA_CS_ROOT="$PWD/../cna-cs" dotnet build cs/StarfieldCourier.GameplayTests.csproj \
  -c Release -p:Engine=CNA
```

On Linux, identify the C API library produced by the embedded CNA build:

```text
build/cna-game/cna/modules/c-api/libcna_c_api.so
```

Run the game or test with that exact library:

```sh
CNA_NATIVE_LIBRARY="$PWD/build/cna-game/cna/modules/c-api/libcna_c_api.so" \
  dotnet cs/bin/StarfieldCourier/CNA/Release/net8.0/starfield_cs.dll

CNA_NATIVE_LIBRARY="$PWD/build/cna-game/cna/modules/c-api/libcna_c_api.so" \
  dotnet cs/bin/StarfieldCourier.GameplayTests/CNA/Release/net8.0/\
starfield_cs_gameplay_tests.dll
```

The corresponding Windows library is `cna_c_api.dll`; use its absolute path
for `CNA_NATIVE_LIBRARY`. CNA.NET's migration guide documents complete source
builds and deployment for Linux, Windows, browser, and Android:
[migrating-xna-games.md](https://github.com/libcna/cna-cs/blob/develop/docs/migrating-xna-games.md).

## C# on FNA

An FNA build is a differential oracle, not a second C# implementation. Build
FNA and set `FNA_FRAMEWORK_PATH` to its compatible `FNA.dll`:

```sh
FNA_FRAMEWORK_PATH=/absolute/path/to/FNA.dll \
  dotnet build cs/StarfieldCourier.csproj -c Release -p:Engine=FNA
FNA_FRAMEWORK_PATH=/absolute/path/to/FNA.dll \
  dotnet build cs/StarfieldCourier.GameplayTests.csproj \
  -c Release -p:Engine=FNA
```

Make FNA's native libraries available through the platform loader, then run:

```sh
LD_LIBRARY_PATH=/absolute/path/to/fna/native \
  dotnet cs/bin/StarfieldCourier/FNA/Release/net8.0/starfield_cs.dll

LD_LIBRARY_PATH=/absolute/path/to/fna/native \
  dotnet cs/bin/StarfieldCourier.GameplayTests/FNA/Release/net8.0/\
starfield_cs_gameplay_tests.dll
```

The native package must provide the SONAMEs requested by its FNA build, for
example `libFNA3D.so.0` and `libFAudio.so.0`. Missing SONAME links are a local
FNA packaging problem, not a change to make in the shared game source.

## Managed graphical validation

Both managed configurations support the same finite validation arguments:

```text
--smoke-frames COUNT --validate-frame --start-sector 1|2|3
--screenshot FILE.ppm
```

For each engine, run all three sectors and require `frame_valid=true`. Example
for CNA.NET:

```sh
for sector in 1 2 3; do
  CNA_NATIVE_LIBRARY="$PWD/build/cna-game/cna/modules/c-api/libcna_c_api.so" \
    dotnet cs/bin/StarfieldCourier/CNA/Release/net8.0/starfield_cs.dll \
      --smoke-frames 1 --validate-frame --start-sector "$sector"
done
```

The finite mode still enters the ordinary XNA `Game.Run()` lifecycle and exits
from `Draw`; it is not a separate simulator. On 2026-10-04 the CNA.NET and FNA
builds both passed their gameplay executable and `frame_valid=true` for all
three sectors in CNA's private real-GPU Weston/Xwayland runner. CNA.NET used
the CNA OpenGL 3.3 renderer; the local FNA build used its Vulkan backend.

## C++ renderer capture parity

The C++ reference additionally supports deterministic PPM captures. Capture
every sector under both CNA renderers:

```sh
mkdir -p build/cna-game/captures
for renderer in OPENGL33 OPENGLES3; do
  for sector in 1 2 3; do
    SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
    CNA_GRAPHICS_RENDERER="$renderer" \
      ./build/cna-game/cpp/starfield_cpp \
        --smoke-frames 1 --validate-frame --start-sector "$sector" \
        --screenshot "build/cna-game/captures/${renderer}-sector${sector}.ppm"
  done
done
```

Reference captures recorded on 2026-09-12 were byte-identical between
OpenGL 3.3 and OpenGL ES 3 on that machine:

| Sector | SHA-256 |
| --- | --- |
| 1 — Starport | `b27fa0afd06276c7e834063a6c5f60dc1bcd9adcce65213f7805b451c27812ef` |
| 2 — Ion Basin | `4194f8122c62c60f5d1a3a1de50b196581a6e158f60a8fe3ee86188ead0ba9f3` |
| 3 — Solar Forge | `750f61951f95bc58ef3c90f247d4d23aef37f76507c7b7e8e933bbdf7ad0ed9d` |

Conformant drivers may rasterize edge details differently; investigate a
cross-machine hash difference visually instead of assuming failure.
