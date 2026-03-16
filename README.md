# HomePad

HomePad is a native Nintendo 3DS Home Assistant dashboard built with devkitPro and libctru.

Initial implementation and project scaffolding were developed with assistance from OpenAI Codex.

It does not render Lovelace, does not emulate Home Assistant cards, and does not embed a web UI. Instead, it translates the same information priorities into a 3DS-native dual-screen app:

- Top screen for passive status and page summaries
- Bottom screen for stylus-friendly controls and navigation
- REST polling only in v1
- Large touch targets for old 3DS and new 3DS hardware

## Quick Start

Latest release:

- `v0.1.0`: `https://github.com/kylie-grace/homepad-3ds/releases/tag/v0.1.0`

Download:

- `homepad-v0.1.0-3dsx.zip`: `https://github.com/kylie-grace/homepad-3ds/releases/download/v0.1.0/homepad-v0.1.0-3dsx.zip`

Install:

1. Download the release zip.
2. Copy `homepad.3dsx` to `sdmc:/3ds/homepad/homepad.3dsx`.
3. Copy `config.template.json` to `sdmc:/3ds/homepad/config.json`.
4. Edit `config.json` with your Home Assistant URL, token, and entity IDs.
5. Launch `HomePad` from the Homebrew Launcher.

## Status

Current state:

- Native UI shell implemented
- Config-driven pages implemented
- Home Assistant REST polling implemented
- Basic service calls implemented for `light`, `switch`, `fan`, `scene`, and `script`
- Build verified locally with current devkitPro `3ds-dev`

Additional project docs:

- `docs/AGENT_HANDOFF.md`
- `docs/ROADMAP.md`

## Features

- Overview page with greeting, time, weather, indoor temperature, presence, and whole-home counts
- Room pages with large touch buttons and room-level highlights
- People page for household presence
- Weather page for current conditions, high/low, wind, and sunrise/sunset
- Quick actions page for scenes, scripts, and favorite toggles
- Touch, D-pad, and button navigation
- Dark modern panel-based aesthetic inspired by Home Assistant dashboards without copying Lovelace layout

## Project Layout

```text
homepad-3ds/
├── Makefile
├── README.md
├── .gitignore
├── config/
│   ├── example_config.json
│   └── homepad.config.template.json
├── include/
│   ├── app.h
│   ├── font8x8_basic.h
│   └── jsmn.h
└── source/
    ├── config.c
    ├── font8x8_basic.c
    ├── ha_client.c
    ├── jsmn.c
    ├── main.c
    └── ui.c
```

## Build

Prerequisites:

- devkitPro
- devkitARM
- libctru
- `DEVKITPRO=/opt/devkitpro`
- `DEVKITARM=/opt/devkitpro/devkitARM`

Build:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
make
```

Build output:

- `homepad.3dsx`
- `homepad.smdh`
- `homepad.elf`

## Install

1. Copy `homepad.3dsx` to `sdmc:/3ds/homepad/homepad.3dsx`.
2. Copy a config file to `sdmc:/3ds/homepad/config.json`.
3. Launch from the Homebrew Launcher.

## Configuration

Start from:

- `config/homepad.config.template.json`

Minimal required fields:

- `home_assistant_url`
- `access_token`
- `weather_entity`
- `indoor_temp_entity`

Recommended fields:

- `display_name`
- `people_entities`
- `favorite_entities`
- `quick_action_entities`
- `rooms`

Field reference:

- `home_assistant_url`: Base URL for Home Assistant, example `https://homeassistant.local:8123`
- `access_token`: Long-lived access token created in Home Assistant
- `display_name`: Greeting name shown on the overview page
- `poll_interval_seconds`: Refresh cadence, clamped to `10` through `300`
- `weather_entity`: Weather entity used for overview and weather page
- `indoor_temp_entity`: Whole-home indoor temperature sensor
- `people_entities`: List of `person.*` entities for the people page
- `favorite_entities`: Overview page action buttons
- `quick_action_entities`: Quick actions page entities
- `rooms`: List of room objects

Room object fields:

- `name`: Short room label
- `temp_sensor`: Main temperature sensor for the room
- `humidity_sensor`: Main humidity sensor for the room
- `control_entities`: Up to 6 important control entities
- `highlight_entities`: Up to 6 passive sensor or status entities

## Example Config Workflow

1. Copy `config/homepad.config.template.json`.
2. Replace `home_assistant_url` and `access_token`.
3. Replace each entity ID with values from your own Home Assistant instance.
4. Keep room names short so they fit on the bottom screen.
5. Put high-value controls in `favorite_entities` and `control_entities`.

## Controls

- Touch: activate buttons on the bottom screen
- D-pad up/down: move button focus
- D-pad left/right: switch rooms while on the room page
- `A`: activate focused button
- `L` and `R`: cycle pages
- `X`: force refresh
- `START`: exit

## Supported Domains

Fully actionable in v1:

- `light`
- `switch`
- `fan`
- `scene`
- `script`

Read-only in v1:

- `weather`
- `sensor`
- `binary_sensor`
- `person`
- `climate`
- `media_player`

## Design Translation

HomePad preserves these dashboard concepts:

- Greeting header
- Large clock presence
- Whole-home status cards
- Presence summary
- Weather focus
- Room summaries
- Utility-oriented quick access

HomePad intentionally re-composes them for 3DS constraints:

- No masonry or grid card emulation
- No Lovelace card rendering
- No embedded web view
- Large, native controls instead of dense card layouts

## Runtime Notes

- If config is missing, the app stays usable enough to show setup instructions on-screen
- Polling errors are shown in the status area instead of crashing the app
- Base URLs are normalized to avoid trailing-slash mistakes
- Poll interval is clamped for safer battery and network behavior
- HTTPS certificate verification is currently disabled for local-network practicality

## Limitations

- No websocket updates in v1
- No climate control actions yet
- No camera snapshots yet
- No real icon pack yet
- Uses a built-in bitmap font and simple software UI primitives
- Weather detail quality depends on attributes exposed by your chosen weather entity
- Sunrise and sunset currently come from `sun.sun` when available
- Service payloads are intentionally simple and only cover common actions

## Roadmap

- Climate controls and preset actions
- Utility pages for traffic, printers, and media
- Sparklines and trend views
- Camera snapshot support
- Theme customization
- Faster partial refresh
- Websocket mode
- Better icons and typography

## Publishing Notes

- Repo name: `homepad-3ds`
- App name: `HomePad`
- The codebase is ready for a public repository
- A license file has not been added yet, so choose one before publishing if you want reuse rights to be explicit
