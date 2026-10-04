# CNA Starfield Courier — completed delivery ledger

The repository has a finite scope: C++, C, and C# only. All three graphical
ports are complete. Do not reopen the former plan for Java, TypeScript, Python,
Rust, Go, Swift, Ruby, Common Lisp, or other languages without an explicit new
owner request.

## Final status

| Task | Status | Evidence |
| --- | --- | --- |
| Shared game specification and acceptance contract | **DONE** | Three-sector behavior, visuals, input, audio, and validation are documented under `shared/`. |
| C++ CNA/XNA reference | **DONE** | Ordinary `Game::Run()`, live state and rendering, seven behavior/graphics tests, OpenGL 3.3 and OpenGL ES 3. |
| C implementation | **DONE** | Pure C99 over the public CNA C ABI; lifecycle, input, 3D drawing, audio, gameplay test, and all three graphical sectors. |
| C# implementation on CNA.NET | **DONE** | Shared XNA 4.0 source builds with `CNA.XnaCompat`; gameplay and three graphical sectors pass. |
| Same C# implementation on FNA | **DONE** | The identical `StarfieldGame.cs` builds and passes the same behavior/frame checks against FNA. |
| Build and validation documentation | **DONE** | `README.md` and `BUILDING.md` contain reproducible native, CNA.NET, and FNA commands. |
| Final cleanup | **DONE** | Only the three requested language implementations remain; generated output stays ignored. |

## Architecture boundary

- The C++ port uses CNA's public XNA-style C++ surface and contains no custom
  renderer or detached simulation.
- The C port uses only the published CNA C binding for runtime, lifecycle,
  input, graphics, audio, and media.
- The C# port is ordinary XNA 4.0-style source. MSBuild selects CNA.NET or FNA;
  gameplay code does not select an engine.
- Every graphical validation enters the real runtime lifecycle and renders the
  state used for gameplay and collision.

## Completion evidence (2026-10-04)

- CNA native C/C++ targets build without warnings from the demo sources.
- The private GPU suite passes 11/11 tests: seven C++ and four C.
- CNA.NET application and behavior-test builds pass with zero warnings and
  zero errors.
- FNA application and behavior-test builds pass with zero warnings and zero
  errors.
- The managed gameplay executable passes on both CNA.NET and FNA.
- The shared C# graphical application reports `frame_valid=true` in sectors
  1, 2, and 3 on both CNA.NET and FNA.
- C++ pointer-access updates keep the reference compatible with the current CNA
  effect-technique API.

This ledger is closed. Future changes should be maintenance, regression fixes,
or deliberately requested improvements to these three implementations—not an
automatic expansion to more languages.
