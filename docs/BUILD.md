# Build And Validation

This project currently relies on manual devkitPro validation rather than CI.

## Expected Environment

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
```

Required packages:

- devkitPro
- devkitARM
- libctru / `3ds-dev`
- `make`

## Local Build

From the repo root:

```sh
make clean
make
```

Expected outputs:

- `homepad.3dsx`
- `homepad.elf`
- `homepad.smdh`

## Manual Release Validation

1. Copy `homepad.3dsx` to `sdmc:/3ds/homepad/homepad.3dsx`.
2. Copy `config/homepad.config.template.json` to `sdmc:/3ds/homepad/config.json` and replace it with real values.
3. Launch from the Homebrew Launcher on hardware.
4. Confirm the overview page loads and the status pill reaches `ONLINE`.
5. Force a refresh with `X` and confirm the status line updates successfully.
6. Trigger one low-risk control for a toggle domain, or verify service availability without changing a real device.
7. If you use climate controls, validate mode cycling and target changes on a safe/test climate entity first.
8. Disconnect Wi-Fi or break the URL once and confirm the app surfaces an offline error instead of crashing.

## Runtime Limits To Recheck

- Polling is synchronous and uses an 8 second HTTP timeout.
- HomePad polls configured entities individually with `/api/states/<entity_id>`.
- The configured tracked entity list is capped by `HA3DS_MAX_ENTITIES` (`256` by default).
- HTTPS certificate verification is intentionally disabled for trusted-LAN compatibility.
