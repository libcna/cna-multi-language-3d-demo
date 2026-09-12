# CNA Starfield Courier: headless contract

This file is the small, renderer-independent contract shared by every port.
The graphical shell is intentionally not part of the conformance protocol.

## Coordinate system and state

The simulation uses a right-handed XNA-style world. `x` is horizontal, `y` is
up (always zero in the headless rules), and `z` is depth. The courier starts at
`(0, 0, 0)` in `Title`. The arena is the square `[-10, 10] x [-10, 10]` in
the X/Z plane. The extraction ring is at `(0, -9)`.

The states are `Title = 0`, `Playing = 1`, `Won = 2`, and `Lost = 3`. The
first update without `restart` changes `Title` to `Playing`. `restart` resets
all values to the initial `Title` snapshot and takes precedence over other
input.

## Input and update

An input frame contains `turn` and `forward` in `[-1, 1]`, and boolean `boost`
and `restart` values. Values outside the range are clamped. Turn changes the
player heading at 180 degrees/second; heading zero faces world `-Z`. Forward
movement follows that heading at `4` units/second, or `7` units/second while
boosting. An update clamps seconds to `0..0.25` and movement to the arena. The
simulation is frame-rate independent; callers that need a fixed step should
submit repeated `1/60` updates.

## Objects and scoring

Energy cells have centers `(-6, 0)`, `(0, -5)`, and `(6, 0)` and a collection
radius of `0.9`. The primary moving hazard has radius `1.0`, starts at `(0, 3)`,
and travels on the X axis between `-7` and `7` at `2` units/second. The second
hazard follows the opposite X phase at `Z=-3.5`. A courier collision radius is
`0.75`; touching either hazard (distance at most `1.75`) changes the state to
`Lost`. The time limit is `60` seconds and also causes `Lost`.

After all three cells are collected, entering the extraction ring (distance at
most `1.4`) changes the state to `Won`. A cell is worth `100` points and a win
adds `1000` points. The score and elapsed time stop changing in terminal
states. A loss never awards the win bonus.

## Snapshot and scenarios

Each snapshot is serialized as:

```text
state mask x z elapsed score heading hazard_x secondary_hazard_x
```

with state as the integer above and decimal floats. The legacy six-field
prefix remains stable while graphical ports are being brought up. Required
scenario names are `startup`, `collection`, `hazard`, `win`, `loss`, and
`restart`. Ports may provide a native runner, but the result must contain the
same fields and values within `1e-4` for floating-point values.

The renderer may use XNA 4.0 concepts (`GameTime`, `Vector3`, `Matrix`,
`Model`, effects, vertex/index buffers, textures, `SpriteBatch`, and input
services). EasyGL is the reference Linux backend and OpenGLES is an optional
equivalent backend. No game-facing source may depend on `CNA.Ext`.
