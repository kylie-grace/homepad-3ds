# Roadmap

This roadmap reflects the current HomePad state after the initial public release and first hardening pass.

## Completed

- Native 3DS dual-screen UI shell
- Config-driven overview, room, people, weather, quick, and utilities pages
- Home Assistant REST polling
- Basic actions for `light`, `switch`, `fan`, `scene`, `script`, and `climate`
- Release packaging and public GitHub release
- Repo handoff documentation
- Initial runtime hardening
- Homelab-backed default config template for traffic, printers, weather, family lights, media, and ops status

## Next

### 1. Hardware Validation

Highest priority:

- Test on real old 3DS and new 3DS hardware
- Validate HTTP behavior on actual Wi-Fi/network conditions
- Confirm touch target sizing and text readability on-device
- Validate room/action flows with a real Home Assistant instance

### 2. Utility Depth

Target:

- Expand the generic utilities page into richer summaries when the base app has been validated on hardware

Likely additions:

- printer-specific progress formatting
- media player summaries
- traffic/commute thresholds
- homelab service health rollups

### 3. Better Presentation

Target:

- Improve density and readability without losing stylus-first usability

Likely additions:

- domain glyphs or icons
- improved text truncation/layout
- stronger selected-state visuals
- optional compact status rows

### 4. Better Home Assistant Data Handling

Target:

- Reduce brittle assumptions about entity attributes and service behavior

Likely additions:

- richer weather attribute parsing
- better media player summaries
- deeper climate attribute parsing and presets
- service-specific success/error messages
- partial refresh strategies

### 5. Live Updates

Target:

- Move beyond polling when the base app is stable

Likely additions:

- websocket support
- event-driven state updates
- reduced polling cost for old 3DS hardware

## Longer-Term Stretch Goals

- camera snapshots
- sparklines/history
- theme customization
- richer release packaging
- CI build automation
- per-user layout/theme profiles

## Current Prioritization

Recommended order:

1. Real hardware validation
2. Utility pages
3. UI/icon polish
4. Better HA data depth
5. Websocket/live updates
