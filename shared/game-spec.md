# CNA Starfield Courier — Shared Graphical Game Specification

This is one small real-time third-person 3D game, not a numeric simulation
protocol. Every port must create a CNA game/window, use CNA’s game loop and
input, and draw this scene through CNA’s XNA 4.0-style public API.

## Scene

- World: right-handed XNA coordinates; arena floor is the XZ plane at `Y = 0`.
- Arena: a `20 x 20` square from `-10` through `10`, with a dark blue floor,
  four orange boundary walls/marker posts, and a visible grid or edge rails.
- Player: a cyan low-poly courier drone centered at `(0, 0.6, 0)` on reset.
- Energy cells: three gold glowing low-poly objects at `(-6, 0.7, 0)`,
  `(0, 0.7, -5)`, and `(6, 0.7, 0)`.
- Hazards: at least two visible red moving prisms. The primary hazard traverses
  `X = -7..7` at `Z = 3`, speed `2` world units/second, reversing at endpoints;
  additional hazards use deterministic phase offsets.
- Extraction: a green gate/ring centered at `(0, 0, -9)` and visible from the
  arena. It activates visually after all cells are collected.
- Geometry is procedural: cubes/prisms/planes/rings made from reproducible
  vertex and index data; no large asset or content pipeline is required.

## Camera and rendering

- Third-person chase camera follows the player from offset `(0, 7, 10)` and
  looks at `(player.X, 0.5, player.Z)`.
- Perspective FOV is `45°`; near plane `0.1`; far plane `100`; reference
  presentation is `1280x720`.
- Depth testing is enabled. Lighting may use CNA’s XNA `BasicEffect` when the
  verified API supports it; otherwise use distinct vertex colors while keeping
  the same geometry and camera.
- Game-facing code uses only public XNA 4.0-style CNA APIs. No CNAEXT, OpenGL,
  EasyGL, OpenGLES, or renderer-specific calls appear in the game source.

## Controls

- `W`/Up: move forward in the player’s heading.
- `S`/Down: move backward.
- `A`/Left and `D`/Right: turn left/right.
- `Space`: boost while held.
- `R` or Enter: restart from any terminal state.
- Escape: quit.

## Rules

- State starts in `Title`, enters `Playing` on the first update, and has
  terminal states `Won` and `Lost`.
- Normal speed is `4` units/second; boost speed is `7`; simulation delta is
  clamped to `0..0.25` seconds and interactive updates are fixed at 60 Hz when
  CNA supports a fixed timestep.
- A cell is collected within radius `0.9`. A hazard collision within radius
  `1.75` loses immediately. Reaching the gate within radius `1.4` wins only
  after all three cells are collected. A 60-second timer also loses.
- Restart fully resets player, hazards, timer, score, and collected cells.

## Verification

Interactive play is the primary behavior. An optional deterministic mode may
start from reset, freeze the reference camera, advance scripted input, and
capture frames for visual comparison. It must not replace the normal game.