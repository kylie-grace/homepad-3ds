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
6. Trigger one control each for a toggle domain and a climate entity.
7. Disconnect Wi-Fi or break the URL once and confirm the app surfaces an offline error instead of crashing.

## Runtime Limits To Recheck

- Polling is synchronous and uses an 8 second HTTP timeout.
- `/api/states` parsing currently stores up to `256` entities.
- HTTPS certificate verification is intentionally disabled for trusted-LAN compatibility.
