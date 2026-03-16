# Roadmap

This roadmap reflects the current HomePad state after the initial public release and first hardening pass.

## Completed

- Native 3DS dual-screen UI shell
- Config-driven overview, room, people, weather, and quick pages
- Home Assistant REST polling
- Basic actions for `light`, `switch`, `fan`, `scene`, and `script`
- Release packaging and public GitHub release
- Repo handoff documentation
- Initial runtime hardening

## Next

### 1. Hardware Validation

Highest priority:

- Test on real old 3DS and new 3DS hardware
- Validate HTTP behavior on actual Wi-Fi/network conditions
- Confirm touch target sizing and text readability on-device
- Validate room/action flows with a real Home Assistant instance

### 2. Climate Controls

Target:

- Add practical `climate` actions without making the UI fragile

Likely scope:

- `climate.turn_off`
- `climate.set_hvac_mode`
- simple preset or mode cycling
- optional temperature up/down controls

### 3. Utility Pages

Target:

- Expand beyond rooms and quick actions for utility-heavy dashboards

Likely additions:

- printers page
- media page
- traffic/commute page
- generic utility page fed by config entities

### 4. Better Presentation

Target:

- Improve density and readability without losing stylus-first usability

Likely additions:

- domain glyphs or icons
- improved text truncation/layout
- stronger selected-state visuals
- optional compact status rows

### 5. Better Home Assistant Data Handling

Target:

- Reduce brittle assumptions about entity attributes and service behavior

Likely additions:

- richer weather attribute parsing
- better media player summaries
- climate attribute parsing
- service-specific success/error messages
- partial refresh strategies

### 6. Live Updates

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
2. Climate controls
3. Utility pages
4. UI/icon polish
5. Websocket/live updates
