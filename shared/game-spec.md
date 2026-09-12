# CNA Starfield Courier — Shared Graphical Game Specification

This specification describes one real-time third-person 3D game. Each future
language implementation must create and run the game through CNA or its CNA
binding. The visible game—not a separate command-line simulation—is the shared
reference.

## Presentation and world

- Reference back buffer: `1280 × 720`.
- Coordinate system: right-handed XNA coordinates, with `Y` up and the arena
  floor on the XZ plane.
- Arena: a 20×20 square bounded by `X,Z = -10..10`.
- Floor: dark blue `(25,45,90)` with blue grid lines `(55,90,135)` every two
  world units.
- Boundaries: orange rails `(255,120,20)` and lighter orange marker posts
  `(255,180,50)` at the edges.
- Clear color: deep navy `(4,10,30)`.

All geometry may be procedural. No content-pipeline asset is required.

## Authoritative objects

- Player: cyan courier craft at `(0,0.6,0)` after reset, heading `0` radians.
  Heading zero faces world `-Z`. A lighter cyan nose makes rotation visible.
- Collectibles: three gold energy cells at `(-6,0.7,0)`, `(0,0.7,-5)`, and
  `(6,0.7,0)`.
- Primary hazard: red prism at `(0,0.8,3)`, moving along X at `+2` units/second.
- Secondary hazard: red prism at `(0,0.6,-3.5)`, moving along X at `-2`
  units/second.
- Each hazard reverses at `X=-7` and `X=7`.
- Extraction gate: three green bars centered around `(0,0,-9)`. It uses dark
  green `(20,100,50)` until all cells are collected and bright green
  `(40,255,80)` afterward.

Hazard collision and drawing must read the same hazard objects. Collectible
collision and drawing must read the same collectible objects. The player mesh,
movement, camera, and collision must read the same player position and heading.

## Camera and rendering

- The chase camera target is `(player.X,0.5,player.Z)`.
- With heading zero, the camera is at `(player.X,7,player.Z+10)`. It rotates
  around the player with heading, always remaining ten units behind.
- Perspective vertical field of view: `45°`.
- Near plane: `0.1`; far plane: `100`; aspect ratio: `16:9`.
- World geometry uses depth testing. HUD geometry disables depth testing and
  uses an orthographic 1280×720 projection.
- Rendering uses an XNA-style `BasicEffect` with colored vertices. Game source
  is renderer-independent.
- The HUD shows three collection indicators, remaining-time bar, colored
  run-state indicator, and four seven-segment score digits.

## Controls

- `W` or Up: forward relative to heading.
- `S` or Down: backward relative to heading.
- `A` or Left: turn left.
- `D` or Right: turn right.
- Space: boost while held.
- `R` or Enter: restart from any state.
- Escape: exit through the XNA game lifecycle.

## Simulation and rules

- Run states are `Title`, `Playing`, `Won`, and `Lost`.
- Reset enters `Title`; the first update enters `Playing`.
- Normal movement speed is `4` units/second; boost speed is `7`.
- Turn speed is `π` radians/second.
- Movement is heading-relative: forward delta is
  `(sin(heading), 0, -cos(heading)) × speed × elapsed`.
- Elapsed time is clamped to `0..0.25` seconds per gameplay step, and the CNA
  game requests a fixed 60 Hz timestep.
- Player X and Z remain within `-10..10`.
- A cell is collected when its XZ distance from the player is at most `0.9`.
  Each cell awards `100` points once.
- A hazard collision occurs at XZ distance at most `1.75` and immediately
  enters `Lost`.
- Reaching 60 elapsed seconds enters `Lost`.
- After all three cells are collected, reaching `(0,0,-9)` within XZ radius
  `1.4` enters `Won` and awards `1000` additional points.
- Gameplay fields stop changing in `Won` and `Lost`, except that restart is
  always accepted.
- Restart resets player position and heading, both hazards and velocities,
  collectibles, elapsed time, score, camera, and run state.

## Verification

Behavior tests must exercise the state owned by the actual XNA game class and
assert final values for movement, camera, collection, hazard collision,
timeout, win, terminal-state freezing, and restart. Graphical tests must enter
ordinary `Game::Run()`, render the real scene, and verify a nonblank 1280×720
frame containing the required world and HUD colors. Renderer parity compares
frames captured from those real game runs.
