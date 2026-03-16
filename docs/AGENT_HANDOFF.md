# Agent Handoff

This file is the current working context for future agents operating on this repo.

## Project Identity

- Repo name: `homepad-3ds`
- App name: `HomePad`
- Repo URL: `https://github.com/kylie-grace/homepad-3ds`
- Default branch: `main`

## Product Goal

HomePad is a native Nintendo 3DS Home Assistant dashboard.

Non-goals:

- No Lovelace rendering
- No Home Assistant card emulation
- No embedded web UI

Core product direction:

- Top screen for passive overview/status
- Bottom screen for touch-first controls/navigation
- REST polling in v1
- Lightweight native UI optimized for 3DS constraints

## Current State

Implemented:

- Native libctru app scaffold
- Config-driven page model
- Overview page
- Room page
- People page
- Weather page
- Quick actions page
- REST polling via Home Assistant `/api/states`
- Basic service calls for `light`, `switch`, `fan`, `scene`, `script`
- MIT license
- Public GitHub repo created and pushed

Build status:

- Builds successfully with current local devkitPro `3ds-dev`
- Current output target name is `homepad`
- Verified outputs:
  - `homepad.3dsx`
  - `homepad.elf`
  - `homepad.smdh`

## Toolchain Notes

Expected environment:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
```

Build command:

```sh
make
```

The project Makefile is aligned to current devkitPro layout and includes:

- `$(DEVKITARM)/3ds_rules`
- `CTRULIB` include/lib paths
- app config path: `sdmc:/3ds/homepad/config.json`

## Key Files

- `Makefile`
- `README.md`
- `LICENSE`
- `config/homepad.config.template.json`
- `include/app.h`
- `source/main.c`
- `source/config.c`
- `source/ha_client.c`
- `source/ui.c`

## Runtime/Architecture Notes

### Config

Config is loaded from:

- primary: `sdmc:/3ds/homepad/config.json`
- fallback in repo: `config/homepad.config.template.json`

Config behavior:

- trailing slashes are trimmed from `home_assistant_url`
- `poll_interval_seconds` is clamped to `10..300`

### Networking

Integration method:

- Home Assistant REST API only

Important implementation notes:

- uses `httpc`
- POST support requires HTTP shared memory, allocated in `source/main.c`
- HTTPS certificate verification is currently disabled
- request timeout handling was added
- polling/action failures are surfaced in the UI status line instead of crashing

### UI

Renderer is custom software-drawn UI with:

- bitmap font
- rounded panel primitives
- dark dashboard styling

Current input behavior:

- Touch for bottom-screen buttons
- `A` activate
- `L` / `R` page cycle
- D-pad move focus / switch room
- `X` force refresh
- `START` exit

## Important Product Decisions

- HomePad is inspired by the user’s Home Assistant dashboard information architecture, not its exact layout
- Do not attempt masonry/grid/Lovelace emulation
- Favor reliability and large controls over feature breadth
- v1 remains polling-based; no websocket requirement

## Git / Publishing State

Current notable commits:

- `b07a962` Initial HomePad 3DS dashboard
- `5b5c2d5` Add MIT license

Remote:

- `origin` -> `https://github.com/kylie-grace/homepad-3ds.git`

The remote was switched to HTTPS because GitHub CLI auth worked but SSH push failed due to missing usable GitHub SSH key configuration on this machine.

## Known Limitations

- `climate` is read-only
- `media_player` is read-only
- no websocket updates
- no theme customization
- no icons beyond text presentation
- no camera snapshots
- weather detail depends on available HA entity attributes
- sunrise/sunset currently read from `sun.sun`

## Recommended Next Work

High-value next steps:

1. Add climate actions for common thermostat adjustments
2. Add utility pages for printers, traffic, or media
3. Improve text/layout truncation for long entity names
4. Add optional icons or domain glyphs
5. Add screenshot assets and polish repo presentation
6. Add CI or at least documented release packaging steps
7. Test on real hardware and adjust touch target sizing if needed

## If Another Agent Picks This Up

Start here:

1. Read `README.md`
2. Read this file
3. Read `docs/ROADMAP.md`
4. Build with `make`
5. Inspect `source/ui.c` and `source/ha_client.c`

When making changes:

- preserve the no-Lovelace design constraint
- keep controls large and stylus-friendly
- prefer native/simple rendering over heavy dependencies
- do not remove the config template or public-repo documentation
