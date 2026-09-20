"""
Generate official Ludelo brand assets (1024x1024 PNG, Steam icons, ICO).
Renders at 4x supersampling (4096x4096) and downsamples with high-quality Lanczos filter.
Brand colors:
  Deep Dark: #0B0E14, #151923
  Mint Cyan: #00F5D4
  Electric Indigo: #6C5CE7
  Subtle glow, bevels, and crisp geometry.
"""

import math
import os
from PIL import Image, ImageDraw, ImageFilter

def create_ludelo_logo(output_size=1024):
    scale = 4
    size = output_size * scale
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    
    # Coordinates in scaled canvas
    center = size / 2.0
    
    # 1. Outer rounded squircle/hexagon shield with glow
    # Outer radius
    card_r = int(size * 0.42)
    corner_r = int(size * 0.12)
    card_box = [center - card_r, center - card_r, center + card_r, center + card_r]
    
    # Outer Glow layer (blurred cyan/indigo gradient)
    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow)
    glow_box = [center - card_r - int(size * 0.03), center - card_r - int(size * 0.03),
                center + card_r + int(size * 0.03), center + card_r + int(size * 0.03)]
    glow_draw.rounded_rectangle(glow_box, radius=corner_r + int(size * 0.03), fill=(108, 92, 231, 140))
    glow = glow.filter(ImageFilter.GaussianBlur(radius=int(size * 0.035)))
    canvas.alpha_composite(glow)
    
    # 2. Main Shield Body (Deep glass gradient)
    body = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    body_draw = ImageDraw.Draw(body)
    body_draw.rounded_rectangle(card_box, radius=corner_r, fill=(21, 25, 35, 245), outline=None)
    
    # Add subtle top-to-bottom gradient inside body
    grad = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    for y in range(int(card_box[1]), int(card_box[3])):
        ratio = (y - card_box[1]) / (card_box[3] - card_box[1])
        # Cyan-tinted dark at top, purple-tinted dark at bottom
        r = int(15 + 15 * ratio)
        g = int(25 - 5 * ratio)
        b = int(35 + 20 * ratio)
        line_draw = ImageDraw.Draw(grad)
        line_draw.line([(card_box[0], y), (card_box[2], y)], fill=(r, g, b, 245))
    
    # Mask gradient to card shape
    mask = Image.new("L", (size, size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle(card_box, radius=corner_r, fill=255)
    body.paste(grad, (0, 0), mask)
    
    # 3. Glowing Border on Shield (Gradient from Cyan top-left to Indigo bottom-right)
    border_w = int(size * 0.018)
    border_img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    for y in range(int(card_box[1]), int(card_box[3])):
        ratio = (y - card_box[1]) / (card_box[3] - card_box[1])
        # Interpolate between Mint (#00F5D4) and Indigo (#6C5CE7)
        r = int(0 * (1 - ratio) + 108 * ratio)
        g = int(245 * (1 - ratio) + 92 * ratio)
        b = int(212 * (1 - ratio) + 231 * ratio)
        b_draw = ImageDraw.Draw(border_img)
        b_draw.line([(card_box[0] - border_w, y), (card_box[2] + border_w, y)], fill=(r, g, b, 255), width=1)
        
    border_mask = Image.new("L", (size, size), 0)
    b_mask_draw = ImageDraw.Draw(border_mask)
    b_mask_draw.rounded_rectangle(card_box, radius=corner_r, outline=255, width=border_w)
    body.paste(border_img, (0, 0), border_mask)
    canvas.alpha_composite(body)

    # 4. Stylized Futuristic "L" + Play Portal Emblem
    # The emblem consists of:
    # - A bold vertical spine (left bar of "L") with angled tech cuts
    # - A forward-reaching bottom base (foot of "L")
    # - A dynamic nested "Play" chevron/triangle pointing right within the crook of the L
    emblem = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    
    # Grid definitions for precision
    w_bar = size * 0.115       # thickness of bars
    x0 = center - size * 0.22  # left edge of L
    x1 = x0 + w_bar            # inner edge of vertical bar
    x2 = center + size * 0.22  # right edge of foot of L
    
    y0 = center - size * 0.24  # top of L
    y1 = center + size * 0.24  # bottom of L
    y_foot_top = y1 - w_bar    # top of horizontal foot
    
    # Gradient for the "L" glyph (Mint #00F5D4 to Indigo #6C5CE7)
    glyph_grad = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    for y in range(int(y0), int(y1)):
        ratio = (y - y0) / (y1 - y0)
        r = int(0 * (1 - ratio) + 108 * ratio)
        g = int(245 * (1 - ratio) + 92 * ratio)
        b = int(212 * (1 - ratio) + 231 * ratio)
        g_draw = ImageDraw.Draw(glyph_grad)
        g_draw.line([(0, y), (size, y)], fill=(r, g, b, 255))
        
    # Mask for the "L" shape with modern 45-degree chamfered cuts
    l_mask = Image.new("L", (size, size), 0)
    l_draw = ImageDraw.Draw(l_mask)
    
    chamfer = size * 0.035
    l_poly = [
        (x0 + chamfer, y0),             # top left (chamfered)
        (x1, y0),                       # top right
        (x1, y_foot_top - chamfer),     # inner corner top
        (x1 + chamfer, y_foot_top),     # inner corner bottom
        (x2, y_foot_top),               # foot right top
        (x2, y1 - chamfer),             # foot right chamfer
        (x2 - chamfer, y1),             # foot bottom right
        (x0, y1),                       # foot bottom left
        (x0, y0 + chamfer)              # left edge chamfer
    ]
    l_draw.polygon(l_poly, fill=255)
    
    # 5. Glowing Play Triangle nested inside the crook of "L"
    # Points to the right (Play portal icon)
    # Scaled and placed in the upper-right quadrant formed by L
    tri_x0 = x1 + size * 0.05
    tri_x1 = x2 + size * 0.02
    tri_y_center = (y0 + y_foot_top) / 2.0 - size * 0.015
    tri_h = size * 0.16
    
    tri_poly = [
        (tri_x0, tri_y_center - tri_h),
        (tri_x1, tri_y_center),
        (tri_x0, tri_y_center + tri_h)
    ]
    
    # Draw Play triangle in mask
    l_draw.polygon(tri_poly, fill=255)
    
    # Apply gradient to L + Play triangle
    emblem.paste(glyph_grad, (0, 0), l_mask)
    
    # Add inner shadow / bevel highlights
    # Inner glow for the emblem
    emblem_glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    eg_draw = ImageDraw.Draw(emblem_glow)
    eg_draw.polygon(l_poly, outline=(0, 245, 212, 180), width=int(size * 0.012))
    eg_draw.polygon(tri_poly, outline=(0, 245, 212, 200), width=int(size * 0.012))
    emblem_glow = emblem_glow.filter(ImageFilter.GaussianBlur(radius=int(size * 0.015)))
    canvas.alpha_composite(emblem_glow)
    
    # Overlay emblem itself
    canvas.alpha_composite(emblem)
    
    # Top highlight sheen over emblem
    sheen = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(sheen)
    s_poly = [
        (card_box[0] + corner_r, card_box[1] + border_w),
        (card_box[2] - corner_r, card_box[1] + border_w),
        (card_box[0] + border_w, center - size * 0.05)
    ]
    s_draw.polygon(s_poly, fill=(255, 255, 255, 18))
    sheen = sheen.filter(ImageFilter.GaussianBlur(radius=int(size * 0.02)))
    canvas.alpha_composite(sheen)

    # Downsample to target size using Lanczos
    final_img = canvas.resize((output_size, output_size), Image.Resampling.LANCZOS)
    return final_img

if __name__ == "__main__":
    print("Generating 1024x1024 Ludelo master logo...")
    logo1024 = create_ludelo_logo(1024)
    
    # Save to primary locations
    logo1024.save("gui/res/logo_square_1024.png", "PNG", optimize=True)
    logo1024.save("gui/logo_square_1024.png", "PNG", optimize=True)
    print("Saved gui/res/logo_square_1024.png and gui/logo_square_1024.png")
    
    # Generate 256x256 Steam icon & logo
    steam_icon = logo1024.resize((256, 256), Image.Resampling.LANCZOS)
    steam_icon.save("gui/res/steam_icon.png", "PNG", optimize=True)
    steam_icon.save("gui/res/steam_logo.png", "PNG", optimize=True)
    print("Saved gui/res/steam_icon.png and gui/res/steam_logo.png")
    
    # Generate multi-size app.ico for Windows (16, 32, 48, 64, 128, 256)
    ico_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    logo1024.save("gui/app.ico", format="ICO", sizes=ico_sizes)
    print("Saved gui/app.ico with multi-resolution icon frames")
