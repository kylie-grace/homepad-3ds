#!/usr/bin/env python3
"""Validate HomePad config files against the firmware limits."""

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
APP_H = ROOT / "include" / "app.h"
CONFIGS = [
    ROOT / "config" / "homepad.config.template.json",
    ROOT / "config" / "example_config.json",
]


def load_limits() -> dict[str, int]:
    text = APP_H.read_text(encoding="utf-8")
    limits: dict[str, int] = {}
    for name, value in re.findall(r"#define\s+(HA3DS_MAX_[A-Z_]+)\s+(\d+)", text):
        limits[name] = int(value)
    return limits


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def validate_config(path: Path, limits: dict[str, int]) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    label = path.relative_to(ROOT).as_posix()

    for key in ("home_assistant_url", "access_token", "display_name"):
        require(isinstance(data.get(key), str) and data[key], f"{label}: missing {key}")

    array_limits = {
        "people_entities": "HA3DS_MAX_PEOPLE",
        "favorite_entities": "HA3DS_MAX_FAVORITES",
        "quick_action_entities": "HA3DS_MAX_QUICK_ACTIONS",
        "utility_entities": "HA3DS_MAX_UTILITY_ENTITIES",
    }
    for key, limit_name in array_limits.items():
        values = data.get(key, [])
        require(isinstance(values, list), f"{label}: {key} must be an array")
        require(len(values) <= limits[limit_name], f"{label}: {key} has {len(values)} items, max {limits[limit_name]}")
        for value in values:
            require(isinstance(value, str) and "." in value, f"{label}: bad entity id in {key}: {value!r}")

    rooms = data.get("rooms", [])
    require(isinstance(rooms, list), f"{label}: rooms must be an array")
    require(len(rooms) <= limits["HA3DS_MAX_ROOMS"], f"{label}: rooms has {len(rooms)} items")
    for room in rooms:
        require(isinstance(room, dict), f"{label}: room entries must be objects")
        room_name = room.get("name", "<unnamed>")
        require(isinstance(room.get("name"), str) and room["name"], f"{label}: room missing name")
        for key, limit_name in {
            "control_entities": "HA3DS_MAX_ROOM_CONTROLS",
            "highlight_entities": "HA3DS_MAX_ROOM_HIGHLIGHTS",
        }.items():
            values = room.get(key, [])
            require(isinstance(values, list), f"{label}: {room_name} {key} must be an array")
            require(len(values) <= limits[limit_name], f"{label}: {room_name} {key} has {len(values)} items")

    print(f"PASS {label}: {len(rooms)} rooms, {len(data.get('utility_entities', []))} utility entities")


def main() -> int:
    limits = load_limits()
    for config in CONFIGS:
        validate_config(config, limits)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
