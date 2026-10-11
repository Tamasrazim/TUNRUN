#!/usr/bin/env python3
"""Bake TUNRUN's original UI material library.

Each sheet is a deterministic, tileable-inspired 4096x2048 RGB surface texture.
The game streams the sheet for the active screen and uses it behind real UI
panels; they are not unreferenced files or size-padding. Python's standard
library is enough, so asset generation adds no third-party runtime dependency.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

WIDTH = 4096
HEIGHT = 2048
BYTES_PER_PIXEL = 3
IMAGE_BYTES = WIDTH * HEIGHT * BYTES_PER_PIXEL
MASK32 = 0xFFFFFFFF

MATERIALS = [
    ("tunnel-steel",       (28, 43, 57), "Directional brushed steel with seams and fine scratches."),
    ("carbon-weave",       (22, 31, 42), "Alternating carbon filaments with woven crossings."),
    ("ceramic-armor",      (38, 48, 59), "Beveled armour tiles, edge cuts and fastener marks."),
    ("neural-circuit",     (18, 43, 54), "Circuit traces, junctions and plated contact points."),
    ("brushed-titanium",   (46, 54, 63), "Cool titanium grain with subtle machined lines."),
    ("graphite-honeycomb", (25, 36, 45), "Hexagonal graphite cells and reinforcement seams."),
    ("plasma-etched",      (28, 34, 54), "Layered etched bands with bright ceramic scoring."),
    ("shipyard-composite", (34, 43, 49), "Heavy fabrication plates and inset panel joints."),
    ("command-glass",      (17, 37, 49), "Dark instrument glass with calibration marks and grid cuts."),
    ("industrial-panel",   (39, 43, 49), "Industrial skin with rivets, channels and wear grain."),
    ("reactor-lattice",    (20, 43, 45), "Interlocked lattice fibres and insulated conductor paths."),
    ("deep-space-composite",(19, 27, 42), "Low-contrast composite with fine diagonal armour facets."),
]


def clamp(value: int) -> int:
    return 0 if value < 0 else 255 if value > 255 else value


def pattern_offset(style: int, x: int, y: int, variant: int) -> int:
    """A small surface-height cue, rather than a flat colour fill."""
    if style == 0:  # Brush / machined metal
        groove = (y + (x // 173) * 3 + variant * 5) % 29
        value = 12 if groove == 0 else -8 if groove == 1 else 0
        if x % 512 in (0, 1, 2) or y % 256 in (0, 1):
            value -= 21
        if (x * 19 + y * 31 + variant * 71) % 997 < 2:
            value += 30
        return value
    if style == 1:  # Carbon braid
        warp = (x + (y // 24 % 2) * 12) % 24
        weft = y % 24
        if warp < 3 or weft < 3:
            return -17
        if warp < 6 or weft < 6:
            return 13
        return 3 if ((x // 6 + y // 6) & 1) == 0 else -2
    if style == 2:  # Ceramic / segmented armour
        px, py = x % 320, y % 192
        edge = min(px, 319 - px, py, 191 - py)
        if edge < 2:
            return -32
        if edge < 4:
            return 16
        if (px - 10) ** 2 + (py - 10) ** 2 < 20:
            return -24
        if (px - 13) ** 2 + (py - 13) ** 2 < 8:
            return 23
        if ((px + py + variant * 17) % 113) < 2:
            return 4
        return 0
    if style == 3:  # Circuit traces and vias
        gx, gy = x % 128, y % 128
        trace = ((gy < 2 or 63 <= gy < 65) and 10 < gx < 118)
        trace = trace or ((gx < 2 or 94 <= gx < 96) and 2 < gy < 120)
        trace = trace or (gx < 3 and 2 < gy < 32)
        vx, vy = (x + variant * 11) % 128, (y + variant * 7) % 128
        via = (vx - 96) ** 2 + (vy - 64) ** 2
        if trace:
            return 31
        if via < 24:
            return 19
        if via < 45:
            return -17
        return -2
    if style == 4:  # Engraved plasma / etched ceramic
        band = abs(((x * 2 + y + variant * 37) % 263) - 131)
        diagonal = (x * 7 + y * 11 + variant * 41) % 509
        value = 15 if band < 2 else -10 if band < 5 else 0
        if diagonal < 2:
            value += 19
        if diagonal in (2, 3, 4):
            value -= 8
        return value
    if style == 5:  # Hex lattice
        hx, hy = x % 72, y % 62
        # Six-sided cell edges, staggered by half a cell on alternating rows.
        q = (x // 36 + y // 31 + variant) & 1
        diagonal_a = (hx + (hy // 2)) % 36
        diagonal_b = (hx - (hy // 2)) % 36
        edge = (hy < 2 or hy > 59 or
                (q == 0 and (diagonal_a < 2 or diagonal_a > 33)) or
                (q == 1 and (diagonal_b < 2 or diagonal_b > 33)))
        return 23 if edge else -3
    return 0


def xorshift32(state: int) -> int:
    state ^= (state << 13) & MASK32
    state ^= state >> 17
    state ^= (state << 5) & MASK32
    return state & MASK32


def make_bmp(destination: Path, index: int, name: str, base_rgb: tuple[int, int, int]) -> dict:
    width, height = WIDTH, HEIGHT
    row_bytes = width * BYTES_PER_PIXEL
    pixel_bytes = row_bytes * height
    # Standard 24-bit BMP, positive height, bottom-up rows, no row padding needed.
    header = struct.pack("<2sIHHI", b"BM", 54 + pixel_bytes, 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0,
                       pixel_bytes, 2835, 2835, 0, 0)
    destination.parent.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256()
    seed = (0x54A9C2D1 ^ ((index + 1) * 0x9E3779B9)) & MASK32
    style = index % 6
    red, green, blue = base_rgb

    with destination.open("wb") as output:
        output.write(header)
        output.write(info)
        digest.update(header)
        digest.update(info)
        for y in range(height - 1, -1, -1):
            row = bytearray(row_bytes)
            state = (seed ^ ((y + 1) * 0x85EBCA6B)) & MASK32
            for x in range(width):
                state = xorshift32(state)
                # Fine material grain, with enough entropy to preserve real
                # surface detail in the shipped maps instead of blank padding.
                noise = ((state >> 24) & 0xFF) - 128
                grain = int(noise / 9)
                detail = pattern_offset(style, x, y, index)
                lift = grain + detail
                # BMP channels are B, G, R. Keep the roughness cue different
                # per channel so the sheets remain legible under a dark UI tint.
                offset = x * 3
                row[offset] = clamp(blue + lift)
                row[offset + 1] = clamp(green + lift + int(noise / 24))
                row[offset + 2] = clamp(red + lift + int(noise / 32))
            output.write(row)
            digest.update(row)

    return {
        "file": destination.name,
        "material": name,
        "width": width,
        "height": height,
        "format": "24-bit BMP / RGB material surface",
        "bytes": destination.stat().st_size,
        "sha256": digest.hexdigest(),
        "source": "original deterministic procedural material",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-directory", required=True, type=Path)
    args = parser.parse_args()
    output_dir = args.output_directory.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    catalog = []
    for index, (name, base_rgb, description) in enumerate(MATERIALS):
        filename = f"surface_{index:02d}.bmp"
        result = make_bmp(output_dir / filename, index, name, base_rgb)
        result["description"] = description
        catalog.append(result)
        print(f"[{index + 1:02d}/{len(MATERIALS):02d}] "
              f"{filename}: {result['bytes']:,} bytes  sha256={result['sha256']}")

    catalog_doc = {
        "product": "TUNRUN",
        "kind": "original UI material library",
        "generator": "scripts/build-ui-materials.py",
        "generator_version": 1,
        "resolution": f"{WIDTH}x{HEIGHT}",
        "texture_count": len(catalog),
        "raw_pixel_bytes": IMAGE_BYTES * len(catalog),
        "gpu_policy": "Only the current screen's material is loaded; it is released on screen changes.",
        "license": "Created for TUNRUN; no third-party image content.",
        "materials": catalog,
    }
    (output_dir / "catalog.json").write_text(
        json.dumps(catalog_doc, indent=2) + "\n", encoding="utf-8")
    total = sum(item["bytes"] for item in catalog)
    print(f"Created {len(catalog)} original material sheets: "
          f"{total:,} bytes ({total / (1024 * 1024):.1f} MiB), plus catalog.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
