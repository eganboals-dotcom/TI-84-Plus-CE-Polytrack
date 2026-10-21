# TI-84 Plus CE Pseudo-3D Time-Trial Racer — Design

Date: 2026-10-20

## Intent

A Trackmania / Polytrack-style time-trial racer for the TI-84 Plus CE. The fun is
grinding short tracks for a better time: race the clock, race your own ghost,
restart instantly, find the braking points. The player rejected generic calculator
game ideas and asked for this specifically.

## Goals (user decided)

- **Pseudo-3D** visuals: chase camera behind the car, road drawn in the classic
  segment/scanline style (OutRun-like). Not top-down, not true polygon 3D.
- **5 or more built-in tracks.**
- **Ghost car** of the player's best run on every track.
- **Best time** saved per track.
- **Real braking and driving feel like Polytrack**: gas, brake, steer; braking matters.
- **Program size ≤ 75 KB** (the `.8xp`; shared CE C libraries installed separately
  don't count, but keep the total reasonable).

## Non-goals / anti-goals

- **No track editor or track building** (user explicitly said no).
- No multiplayer, no link-cable play.
- Anti-goal: choppy, unresponsive driving. If it doesn't feel fast and controllable,
  it fails even if every feature exists.
- Anti-goal: slow restarts. Restarting must be instant (no loading screen).
- No medals for now (not requested; easy to add later).

## Gameplay (agreed in conversation; items marked *Claude's call* were proposed and not objected to)

- **Instant restart** key back to the start line. *Claude's call.*
- **Checkpoints** along each track with split times shown green (ahead) / red (behind)
  vs the best run. *Claude's call.*
- Run length roughly **30–60 s** per track; tracks get harder from 1 to 5+.
  *Claude's call.*
- Track features: curves of varying sharpness, hills/crests, faked jumps/dips.
  *Claude's call.*
- **Driving model:**
  - Gas, brake, steer. Holding brake at (near) standstill reverses.
  - Grip by default: correct speed through a corner holds the line.
  - Entering a corner too fast pushes the car wide / costs time; braking or lifting
    first lets you hit the apex. Learning braking points is the skill.
  - Light, controllable drift when turning hard at high speed — loses a bit of speed,
    never a spin-out.
  - Hitting the track edge/walls slows the car sharply; it does not end the run.
  - Sense of speed: road stripes and roadside scenery rushing by, on-screen speedometer.
- **Ghost:** the best run is replayed as a semi-transparent/outlined car on the road.
  Shown on every track that has a saved best.

## Constraints

- Hardware: TI-84 Plus CE (eZ80 ~48 MHz, 320×240 16-bit-capable LCD usually driven
  in 8-bit palette mode, no FPU, ~150 KB user RAM, archive flash for persistence).
- Language/toolchain: **C with the CE C/C++ toolchain** (CEdev), using the standard
  CE libraries (graphx, keypadc, fileioc). *Claude's call; user accepted.*
- User's calculator runs **OS 5.4**, so native C programs run directly; no jailbreak
  needed. The user should stay on 5.4 (updating to 5.5+ blocks C programs).
- Deliverable: a `.8xp` the user sends with TI Connect CE, plus instructions to install
  the CE C libraries (clibs) if they don't have them.
- Saves: best time + ghost per track stored in an **AppVar in archive** so they
  survive a RAM reset. A ghost is ~1 byte of input per physics tick (~2 KB per track
  at 30 Hz for 60 s). *Claude's call.*
- Ghost replay must be exact, so physics should be **deterministic fixed-point** at a
  fixed tick rate, decoupled from render frame rate if needed. (Recording car position
  per tick is an acceptable fallback if size allows.)
- Target framerate: smooth enough to feel fast — aim ~30 fps, never below ~20 fps.
  Budget and test against the **slowest CE hardware revision** (rev M+ / Python
  edition, extra memory wait states), since the user's revision is unknown.
  *Claude's call.*

## Controls (*Claude's call*, refine during build)

- Arrow left/right: steer. Gas and brake on two keys near the arrows (e.g. `2nd` = gas,
  `alpha` = brake, or up/down arrows). Restart on a single easy key. `clear` = back to menu.

## Left to the builder

- Exact key mapping (keep it comfortable for one-handed thumbs on the arrows + one
  other hand).
- Track data format, track lengths, and the actual 5+ track layouts.
- Rendering technique details (segment projection, palette, sprite style, scenery).
- Physics constants and tuning — tune for feel.
- Menu/HUD layout: track select showing best time per track; in-race timer, speed,
  checkpoint splits.
- How the ghost is drawn (dithered/outlined/palette-swapped).
- Testing approach (CEmu emulator for on-PC testing is the obvious choice).
