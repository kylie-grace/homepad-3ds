# Emulator Verification

Last checked: 2026-07-02

## Build

HomePad builds successfully from the NAS worktree with the F: devkitPro/MSYS2 toolchain:

```sh
source /etc/profile.d/devkit-env.sh
cd //Truenas/Kylie-Grace/Development/homepad-3ds-finished
make clean && make
```

The build produces `homepad.3dsx`, `homepad.elf`, and `homepad.smdh`.

## Azahar Result

Azahar 2125.1.3 MSYS2 was installed under:

```text
F:\Azahar\azahar-windows-msys2-2125.1.3\
```

The official devkitPro `graphics/printing/hello-world` `.3dsx` example launches and renders in this emulator build. HomePad also launches and renders a readable dashboard frame after two runtime fixes:

- `AppState` is stored statically instead of on the 3DSX stack.
- `config_load()` stores JSON parse tokens on the heap instead of on the 3DSX stack.
- Font glyph columns are flipped to match `font8x8_basic` bit order on the 3DS framebuffer.

Captured proof images are intentionally untracked and live at:

```text
docs/screenshots/
```

The fallback-config passing screenshot is:

```text
docs/screenshots/azahar-msys2-homepad-readable.png
```

This proves the app gets past launch, initializes graphics/config, and renders the fallback dashboard in Azahar.

## Live Home Assistant Proof

A real token-bearing config was also installed into Azahar's virtual SD path:

```text
C:\Users\Kylie-Grace\AppData\Roaming\Azahar\sdmc\3ds\homepad\config.json
```

That file is intentionally not tracked. It must be written as UTF-8 without a BOM because the embedded JSON parser expects the first byte to be `{`.

The live Home Assistant proof screenshots are:

```text
docs/screenshots/azahar-homepad-live-tracked-entities-polished.png
docs/screenshots/azahar-homepad-abode-design-pass.png
```

The README crop is committed at:

```text
docs/assets/homepad-live-abode-cropped.png
```

Those runs show HomePad online against `http://192.168.1.212:8123`, with tracked entities loaded, live weather/temperature, active lights/devices, household presence, and populated favorite controls.
