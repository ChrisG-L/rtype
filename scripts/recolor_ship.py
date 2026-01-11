#!/usr/bin/env python3
"""
Ship Sprite Recoloring Script
Generates color variants of the spaceship sprite using hue rotation.

Usage:
    python scripts/recolor_ship.py

Output:
    assets/spaceship/Ship1.png  (original - green)
    assets/spaceship/Ship2.png  (red)
    assets/spaceship/Ship3.png  (blue)
    assets/spaceship/Ship4.png  (yellow)
    assets/spaceship/Ship5.png  (violet)
    assets/spaceship/Ship6.png  (cyan)
"""

from PIL import Image
import colorsys
import os
import sys

# Color definitions with hue shifts (0-360 degrees)
# Original sprite is green (~120 degrees hue)
COLORS = {
    1: {"name": "green",  "hue_shift": 0},      # Original
    2: {"name": "red",    "hue_shift": -120},   # Green -> Red
    3: {"name": "blue",   "hue_shift": 120},    # Green -> Blue
    4: {"name": "yellow", "hue_shift": -60},    # Green -> Yellow
    5: {"name": "violet", "hue_shift": 180},    # Green -> Violet/Magenta
    6: {"name": "cyan",   "hue_shift": 60},     # Green -> Cyan
}

def shift_hue(image: Image.Image, hue_shift_degrees: float) -> Image.Image:
    """
    Shift the hue of an image by a given number of degrees.
    Preserves transparency (alpha channel).
    """
    if hue_shift_degrees == 0:
        return image.copy()

    # Convert to RGBA if not already
    if image.mode != 'RGBA':
        image = image.convert('RGBA')

    # Get pixel data
    pixels = image.load()
    width, height = image.size

    # Create new image
    result = Image.new('RGBA', (width, height))
    result_pixels = result.load()

    # Type assertions for Pylance
    assert pixels is not None
    assert result_pixels is not None

    hue_shift = hue_shift_degrees / 360.0  # Convert to 0-1 range

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]

            # Skip fully transparent pixels
            if a == 0:
                result_pixels[x, y] = (r, g, b, a)
                continue

            # Convert RGB to HLS (Hue, Lightness, Saturation)
            h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)

            # Shift hue (wrap around 0-1)
            h = (h + hue_shift) % 1.0

            # Convert back to RGB
            r_new, g_new, b_new = colorsys.hls_to_rgb(h, l, s)

            result_pixels[x, y] = (
                int(r_new * 255),
                int(g_new * 255),
                int(b_new * 255),
                a
            )

    return result

def main():
    # Paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    assets_dir = os.path.join(project_root, "assets", "spaceship")

    source_path = os.path.join(assets_dir, "Ship1.png")

    # Check source exists
    if not os.path.exists(source_path):
        print(f"Error: Source sprite not found at {source_path}")
        sys.exit(1)

    # Load source image
    print(f"Loading source: {source_path}")
    source_image = Image.open(source_path)
    print(f"  Size: {source_image.size}")
    print(f"  Mode: {source_image.mode}")

    # Generate color variants
    print("\nGenerating color variants:")
    for ship_num, color_info in COLORS.items():
        output_path = os.path.join(assets_dir, f"Ship{ship_num}.png")

        if ship_num == 1:
            # Keep original
            print(f"  Ship{ship_num}.png - {color_info['name']} (original, skipping)")
            continue

        # Apply hue shift
        recolored = shift_hue(source_image, color_info['hue_shift'])

        # Save
        recolored.save(output_path, 'PNG')
        print(f"  Ship{ship_num}.png - {color_info['name']} (hue shift: {color_info['hue_shift']})")

    print(f"\nDone! Generated {len(COLORS) - 1} color variants in {assets_dir}")
    print("\nColor mapping:")
    for ship_num, color_info in COLORS.items():
        print(f"  Ship{ship_num} = {color_info['name'].upper()}")

if __name__ == "__main__":
    main()
