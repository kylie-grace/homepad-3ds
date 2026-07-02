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

This stack does not prove that the app renders in an emulator. It proves the firmware artifact is reproducibly built. Runtime proof is recorded separately in:

```text
docs/EMULATOR_VERIFICATION.md
```

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

This confirmed the Portainer/Docker build gate worked from Gitea commit `d703daa`.

## 2026-07-02 Runtime-Fix Run

After the Azahar MSYS2 launch/rendering fixes, the same Docker API test was run again from pushed Gitea commit `d14c848`.

Result:

```text
d14c848
PASS config/homepad.config.template.json: 5 rooms, 18 utility entities
PASS config/example_config.json: 5 rooms, 18 utility entities
built ... homepad.smdh
built ... homepad.3dsx
PASS HomePad build artifacts created
```

This confirms the current Gitea revision still passes the repeatable Docker build gate after the emulator runtime fixes.
