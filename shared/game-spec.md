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
- Each sector uses the following deterministic environment:

| Sector | Sky | Floor | Grid | Boundary / accent | Landmarks |
| --- | --- | --- | --- | --- | --- |
| 1 — Starport | `(4,10,30)` | `(25,45,90)` | `(55,90,135)` | `(255,120,20)` / `(0,175,225)` | blue runway and orange edge beacons |
| 2 — Ion Basin | `(13,5,27)` | `(55,30,85)` | `(130,65,150)` | `(30,220,220)` / `(205,80,255)` | diagonal ion traces and cyan/magenta crystal towers beyond the boundary |
| 3 — Solar Forge | `(25,5,8)` | `(72,27,25)` | `(150,58,35)` | `(255,185,35)` / `(255,75,35)` | glowing floor channels and forge stacks beyond the boundary |

World geometry is procedural and uses no renderer-specific asset path.

## Authoritative objects

- The player is a cyan courier craft. Heading zero faces world `-Z`; a lighter
  cyan nose makes heading visible.
- Every sector has three gold energy cells and at least two active hazards:

| Sector | Player start | Cell positions | Hazards | Gate |
| --- | --- | --- | --- | --- |
| Starport | `(0,0.6,0)` | `(-6,0.7,0)`, `(0,0.7,-5)`, `(6,0.7,0)` | horizontal from `(0,0.8,3)` at `+2`; horizontal from `(0,0.6,-3.5)` at `-2`; both range `7` | `(0,0,-9)` |
| Ion Basin | `(0,0.6,8)` | `(-7,0.7,6)`, `(7,0.7,-1)`, `(-5,0.7,-6)` | vertical around `(-2,0.8,0)` at `+2`, range `7`; horizontal around `(0,0.7,-2)` at `-2`, range `7`; orbit around `(2,0.9,2)`, radius `2.5` | `(8,0,-8)` |
| Solar Forge | `(0,0.6,8)` | `(-7,0.7,-5)`, `(0,0.7,-6)`, `(6,0.7,6)` | horizontal around `(0,0.8,4)` at `+2.5`, range `8`; vertical around `(-4,0.7,0)` at `-2.4`, range `8`; orbit around `(3,1,-3)`, radius `3` | `(-8,0,-8)` |

- Horizontal and vertical hazards reverse at `origin ± range`. Orbiters advance
  phase by `1.35` radians/second and write their calculated XZ location back to
  the authoritative hazard position before collision and drawing.
- The extraction gate uses dark
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
- The HUD shows three collection indicators, per-sector remaining time,
  sector name/progress, colored run-state indicator, and four score digits.
- A short non-blocking banner introduces each environment without hiding the
  craft or pausing gameplay.
- `Won` and `Lost` keep rendering a subtly animated presentation layer over
  the frozen gameplay state. A large terminal panel names the result and says
  `PRESS R OR ENTER TO RESTART`, so a completed run cannot resemble a hang.

## Audio

- Collecting a cell plays a short rising confirmation chime; successive cells
  rise slightly in pitch.
- Hazard impact or timeout plays the loss cue, entering an unlocked gate plays
  a sector-clear/final-victory cue, and restart plays a short button cue.
- The four original PCM16 WAV cues contain no third-party recordings. Production
  code loads them in `LoadContent` using XNA `SoundEffect::FromStream` and plays
  them using XNA `SoundEffect::Play`.
- The CC0/Public Domain `Outer Space Loop` by wipics plays quietly in the
  background. It is loaded with XNA `Song::FromUri`, played by XNA
  `MediaPlayer`, and repeats continuously. Complete provenance, license, and
  checksum are stored beside the audio assets.

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
- Every sector has its own 60-second timer; reaching 60 seconds enters `Lost`.
- After all three local cells are collected, reaching that sector's gate within
  XZ radius `1.4` advances immediately. Clearing sectors one and two awards
  `500` points each, preserves score, and loads the next layout with a fresh
  timer and three uncollected cells. Clearing sector three awards `1000` and
  enters `Won`. A perfect campaign score is `2900`.
- Gameplay fields stop changing in `Won` and `Lost`, except that restart is
  always accepted. Presentation-only pulsing continues behind the terminal
  panel to make the responsive game loop visible.
- Restart resets the entire campaign to Starport: player, heading, all hazard
  positions/motion, collectibles, sector timer, score, camera, and run state.

## Verification

Behavior tests must exercise the state owned by the actual XNA game class and
assert final values for movement, camera, collection, all three hazard motion
types, sector transitions and score preservation, timeout, final win,
terminal-state freezing, and restart. Graphical tests must enter
ordinary `Game::Run()`, render the real scene, and verify a nonblank 1280×720
frame containing the required world and HUD colors for every sector. Renderer
parity compares frames captured from those real game runs.
