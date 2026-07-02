# Portainer Testing

This project is not done until it launches, but Docker/Portainer is still useful as the first repeatable gate:

1. Validate the Home Assistant config files against firmware limits.
2. Build the 3DSX/SMDH/ELF artifacts in a clean devkitPro container.
3. Preserve artifacts in a Docker volume so they can be copied into emulator/device testing.

## Stack

Deploy this Compose file as a Portainer stack:

```text
docker/portainer-test-stack.yml
```

Use the Gitea repository as the stack source if possible:

```text
https://gitea.katrax.xyz/katrax/homepad-3ds.git
```

The stack runs two one-shot services:

- `config-test`: parses `config/homepad.config.template.json` and `config/example_config.json`.
- `firmware-build`: builds `homepad.3dsx`, `homepad.elf`, and `homepad.smdh` with `devkitpro/devkitarm`.

Successful logs should include:

```text
PASS config/homepad.config.template.json
PASS config/example_config.json
PASS HomePad build artifacts created
```

## Runtime Proof

This stack does not prove that the app renders in an emulator. It proves the firmware artifact is reproducibly built. Runtime proof still needs one of these:

- a 3DS emulator/container path that can run the official devkitPro hello-world `.3dsx` first;
- a packaged CIA/CXI build target and emulator install test;
- physical 3DS homebrew launch with `sdmc:/3ds/homepad/config.json`.

If the emulator cannot run the official devkitPro sample, HomePad runtime screenshots from that emulator are not meaningful yet.

## 2026-07-02 Run

The test was run through the Docker API on `docker-test` (`192.168.1.119`) after increasing PVE2 LXC 104 rootfs from `4G` to `7G`.

Result:

```text
PASS config/homepad.config.template.json: 5 rooms, 18 utility entities
PASS config/example_config.json: 5 rooms, 18 utility entities
built ... homepad.3dsx
PASS HomePad build artifacts created
```

Docker-produced artifacts were copied to the untracked local folder:

```text
docker-artifacts/artifacts/
```

This confirms the Portainer/Docker build gate works from Gitea commit `d703daa`. The remaining blocker is still runtime launch/rendering in an emulator or on physical 3DS hardware.
