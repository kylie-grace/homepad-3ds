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

Azahar 2125.1.3 was launched from `F:\Azahar\azahar-windows-msvc-2125.1.3\azahar.exe` with the built `homepad.3dsx`.

The emulator accepted the file but did not reach a visible app frame. It remained on the launch overlay or a black render surface. The same behavior reproduced with the official devkitPro `graphics/printing/hello-world` `.3dsx` example, so the current blocker appears to be Azahar's loose `.3dsx` homebrew path in this local emulator setup rather than HomePad-specific startup code.

Captured proof images are intentionally untracked and live at:

```text
docs/screenshots/
```

## Follow-Up

For emulator proof of the rendered dashboard, use a 3DS emulator/build path that can run devkitPro `.3dsx` homebrew examples successfully, or add a packaged CIA/CXI build target once a suitable packager is available in the toolchain.
