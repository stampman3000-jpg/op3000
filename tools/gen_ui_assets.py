#!/usr/bin/env python3
"""Procedural 4x-supersampled 1986 Japanese Arcade Cabinet / Realistic VFD-CRT UI generator for OP3000.

Aesthetic:
  - Glossy 1986 Arcade Racing-Red molded T-molding side rails with recessed slot shadows
  - Warm powder-coated dark charcoal-steel control deck with fine metallic enamel grain
  - Extruded brushed-aluminum top/bottom cabinet trim & countersunk stainless carriage bolts
  - Iconic 1986 Sega/OutRun layered racing-stripe OP3000 marquee nameplate
  - Deeply sculpted curved-glass VFD/CRT monitor housings with tube curvature tint,
    aperture-grille scanlines, subtle phosphor mesh, and realistic curved-glass specular reflection
  - Authentic 1980s fluted synth/arcade knobs with glossy coloured anodized caps
    (Arcade Red, Vintage Gold, Sega Teal, Brushed Silver) + recessed calibrated vertical faders
  - Vintage silkscreened section plates with colour-coded headers & corner racing stripes
"""
import json
import math
import os
import random
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIN_DIR = os.path.join(ROOT, "plugins", "op_arcade", "skins")
MANIFEST = os.path.join(ROOT, "tools", "ui_manifest.json")

W, H = 1280, 628
SS = 2  # 2x supersampling for 1280x628 panels (4x for knobs/faders/meters)

# Authentic 1986 Japanese Arcade Cabinet & Synth Hardware Palette
CREAM       = (238, 232, 216, 255)
CREAM_DIM   = (168, 162, 148, 255)
DARK_INK    = (22, 20, 19, 255)
RULE_LINE   = (138, 132, 120, 255)

ARC_RED     = (204, 54, 46, 255)
ARC_RED_HI  = (236, 86, 72, 255)
ARC_ORANGE  = (224, 114, 42, 255)
ARC_GOLD    = (218, 164, 48, 255)
ARC_TEAL    = (46, 144, 152, 255)
ARC_CREAM_B = (218, 211, 194, 255)

# Realistic Vacuum Fluorescent Display (VFD) / Phosphor CRT Colours
VFD_MINT    = (92, 242, 204, 255)
VFD_CYAN    = (74, 224, 238, 255)
VFD_AMBER   = (252, 194, 68, 255)
VFD_CORAL   = (248, 96, 76, 255)


def load_font(px, bold=True):
    candidates = (
        ["/System/Library/Fonts/HelveticaNeue.ttc", "/System/Library/Fonts/Supplemental/Arial Bold.ttf"]
        if bold else
        ["/System/Library/Fonts/HelveticaNeue.ttc", "/System/Library/Fonts/Supplemental/Arial.ttf"]
    )
    for p in candidates:
        if os.path.exists(p):
            try:
                return ImageFont.truetype(p, int(px), index=1 if (p.endswith(".ttc") and bold) else 0)
            except Exception:
                try:
                    return ImageFont.truetype(p, int(px))
                except Exception:
                    pass
    return ImageFont.load_default()


def draw_text(img, text, x, y, size_px, fill=CREAM, align="center", bold=True, track=0.05):
    font = load_font(size_px * SS, bold=bold)
    tmp = Image.new("RGBA", (1, 1), (0, 0, 0, 0))
    tdr = ImageDraw.Draw(tmp)
    chars = list(text)
    widths = []
    for ch in chars:
        bb = tdr.textbbox((0, 0), ch, font=font)
        widths.append(bb[2] - bb[0])
    pad_tr = int(round(size_px * SS * track))
    total_w = sum(widths) + pad_tr * max(0, len(chars) - 1)
    if align == "center":
        cx = x - total_w / 2.0
    elif align == "right":
        cx = x - total_w
    else:
        cx = x
    cy = y - (size_px * SS) * 0.56
    dr = ImageDraw.Draw(img)
    for ch, cw in zip(chars, widths):
        dr.text((cx, cy), ch, font=font, fill=fill)
        cx += cw + pad_tr


def make_op3000_chassis(seed=1986):
    """1986 Japanese Arcade Cabinet: glossy Arcade-Red T-molding side rails + powder-coated charcoal steel deck."""
    w, h = W * SS, H * SS
    rng = random.Random(seed)
    base = Image.new("RGBA", (w, h), (31, 30, 32, 255))
    dr = ImageDraw.Draw(base)

    # Warm overhead arcade marquee studio gradient across dark steel control deck
    for y in range(h):
        t = y / float(h)
        d = int(round(8.5 * (1.0 - abs(t - 0.25) * 1.42)))
        dr.line([(0, y), (w, y)], fill=(29 + d, 29 + d, 31 + d, 255))

    # Authentic fine vintage powder-coat enamel micro-grain
    grain = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    gdr = ImageDraw.Draw(grain)
    for y in range(0, h, 3):
        v = rng.randint(-4, 4)
        if v > 0:
            gdr.line([(0, y), (w, y)], fill=(255, 248, 236, v * 2))
        elif v < 0:
            gdr.line([(0, y), (w, y)], fill=(0, 0, 0, (-v) * 3))
    base.alpha_composite(grain)

    # Top & bottom extruded brushed-aluminum cabinet rails
    dr = ImageDraw.Draw(base)
    dr.rectangle([0, 0, w, 7 * SS], fill=(20, 20, 22, 255))
    dr.line([(0, 7 * SS), (w, 7 * SS)], fill=(94, 92, 88, 255), width=2 * SS)
    dr.line([(0, 9 * SS), (w, 9 * SS)], fill=(12, 12, 13, 255), width=1 * SS)

    dr.rectangle([0, h - 7 * SS, w, h], fill=(18, 18, 20, 255))
    dr.line([(0, h - 7 * SS), (w, h - 7 * SS)], fill=(76, 74, 70, 255), width=2 * SS)

    # Left & Right Sculpted 1986 Arcade Racing-Red Molded T-Molding Side Rails (22px each)
    rail_w = 22 * SS
    for side in (0, 1):
        x0 = 0 if side == 0 else w - rail_w
        for dx in range(rail_w):
            u = dx / float(rail_w - 1)
            # Glossy rounded vinyl T-molding profile with crisp specular ridge
            cyl = math.sin(u * math.pi)
            spec = math.exp(-((u - (0.36 if side == 0 else 0.64)) / 0.14) ** 2)
            r = int(max(45, min(255, 86 + 114 * cyl + 62 * spec)))
            g = int(max(12, min(210, 18 + 34 * cyl + 58 * spec)))
            b = int(max(12, min(200, 16 + 28 * cyl + 50 * spec)))
            dr.line([(x0 + dx, 0), (x0 + dx, h)], fill=(r, g, b, 255))
        # Dark recessed rubber slot + cast shadow where T-molding meets the steel deck
        slot_x = rail_w if side == 0 else (w - rail_w - 5 * SS)
        dr.rectangle([slot_x, 0, slot_x + 5 * SS, h], fill=(10, 10, 11, 235))
        trim_x = (rail_w + 5 * SS) if side == 0 else (w - rail_w - 7 * SS)
        dr.line([(trim_x, 0), (trim_x, h)], fill=(72, 70, 68, 210), width=2)

    # 4 Countersunk Stainless Steel Arcade Control-Deck Carriage Bolts (strictly clear of CRT hoods!)
    for bx, by in [(34, 18), (W - 34, 18), (34, H - 14), (W - 34, H - 14)]:
        cx, cy = bx * SS, by * SS
        ro, ri = int(5.2 * SS), int(3.4 * SS)
        dr.ellipse([cx - ro, cy - ro, cx + ro, cy + ro],
                   fill=(14, 14, 16, 255), outline=(66, 64, 60, 255), width=2)
        dr.ellipse([cx - ri, cy - ri, cx + ri, cy + ri],
                   fill=(128, 126, 120, 255), outline=(188, 184, 174, 255), width=2)
        # Dome highlight
        dr.ellipse([cx - ri + 2, cy - ri + 2, cx, cy], fill=(228, 224, 214, 190))

    return base


def draw_op3000_marquee(img, dr, x, y, subtitle, width=296):
    """Iconic 1986 Sega/OutRun layered sunset racing-stripe OP3000 cabinet marquee emblem."""
    x0, y0 = x * SS, y * SS
    x1 = (x + width) * SS
    # Iconic 4-band 1986 racing stripe accent above the emblem
    stripes = [ARC_CREAM_B, ARC_GOLD, ARC_ORANGE, ARC_RED]
    for i, col in enumerate(stripes):
        sy = y0 - (4 - i) * int(2.2 * SS)
        dr.line([(x0, sy), (x1, sy)], fill=col, width=max(2, int(2.0 * SS)))

    # Tight, unmistakable embossed "OP3000" badge (brushed ivory "OP" + Arcade-Red "3000")
    draw_text(img, "OP", (x + 2) * SS + 2, (y + 20) * SS + 3, 28, fill=(10, 10, 11, 220), align="left", bold=True, track=0.04)
    draw_text(img, "OP", (x + 2) * SS,     (y + 20) * SS,     28, fill=CREAM,            align="left", bold=True, track=0.04)
    draw_text(img, "3000", (x + 48) * SS + 2, (y + 20) * SS + 3, 28, fill=(10, 10, 11, 220), align="left", bold=True, track=0.05)
    draw_text(img, "3000", (x + 48) * SS,     (y + 20) * SS,     28, fill=ARC_RED_HI,       align="left", bold=True, track=0.05)

    # Small metallic "DUAL-CHIP FM" stamp pill next to OP3000
    px0, py0, px1, py1 = (x + 144) * SS, (y + 9) * SS, (x + 288) * SS, (y + 29) * SS
    dr.rounded_rectangle([px0, py0, px1, py1], radius=3 * SS,
                         fill=(20, 20, 22, 235), outline=ARC_GOLD, width=int(1.4 * SS))
    draw_text(img, "DUAL-CHIP FM", (x + 216) * SS, (y + 19) * SS, 10.5, fill=ARC_GOLD, align="center", bold=True, track=0.10)

    # Crisp vintage silkscreened subtitle underneath
    draw_text(img, subtitle, (x + 2) * SS, (y + 42) * SS, 10.0, fill=CREAM_DIM, align="left", bold=True, track=0.07)


def draw_realistic_crt(img, x, y, w, h, radius=10, hood=6, divider_x=None):
    """Deeply sculpted 1986 curved-glass CRT / VFD monitor bezel with glass reflection sheen & scanlines."""
    x0, y0, x1, y1 = x * SS, y * SS, (x + w) * SS, (y + h) * SS
    hd = hood * SS
    r = radius * SS

    # 1. Molded matte-charcoal outer CRT plastic bezel hood (3D lit bevel)
    dr = ImageDraw.Draw(img)
    dr.rounded_rectangle([x0 - hd, y0 - hd, x1 + hd, y1 + hd], radius=r + hd // 2,
                         fill=(24, 24, 26, 255), outline=(78, 76, 72, 255), width=2 * SS)
    # Recessed hood inner shadow chamfer (bottom/right light catch, top/left deep recess shadow)
    dr.rounded_rectangle([x0 - hd + 3, y0 - hd + 3, x1 + hd - 3, y1 + hd - 3], radius=r + hd // 2 - 2,
                         outline=(12, 12, 14, 255), width=2 * SS)
    dr.line([(x0 - hd + r, y1 + hd - 2), (x1 + hd - r, y1 + hd - 2)], fill=(96, 94, 88, 190), width=2)

    # 2. Curved CRT Glass Tube Layer (rendered locally so glass vignette + sheen stay strictly clipped!)
    gw, gh = x1 - x0, y1 - y0
    tube = Image.new("RGBA", (gw, gh), (0, 0, 0, 0))
    tdr = ImageDraw.Draw(tube)

    # Dark smoked emerald-charcoal CRT phosphor glass base
    tdr.rounded_rectangle([0, 0, gw - 1, gh - 1], radius=r, fill=(8, 15, 16, 255))

    # Subtle curved-tube radial glow (slightly brighter green-teal phosphor bed in center, dark curved edges)
    glow_im = Image.new("RGBA", (gw, gh), (0, 0, 0, 0))
    gdr = ImageDraw.Draw(glow_im)
    gdr.ellipse([int(gw * 0.08), int(gh * 0.06), int(gw * 0.92), int(gh * 0.94)],
                fill=(18, 42, 40, 125))
    glow_im = glow_im.filter(ImageFilter.GaussianBlur(radius=max(12, gh // 3)))
    tube.alpha_composite(glow_im)

    # Authentic CRT aperture-grille horizontal scanlines + subtle vertical shadow mask grid
    tdr = ImageDraw.Draw(tube)
    for sy in range(2, gh - 2, 4):
        tdr.line([(2, sy), (gw - 2, sy)], fill=(0, 0, 0, 88), width=1)
    for sx in range(4, gw - 4, 8):
        tdr.line([(sx, 2), (sx, gh - 2)], fill=(42, 92, 86, 16), width=1)

    # Optional etched internal VFD instrument bay divider
    if divider_x is not None:
        dx = int((divider_x - x) * SS)
        tdr.line([(dx, 10), (dx, gh - 10)], fill=(54, 84, 82, 150), width=2)

    # 3. Realistic Curved-Glass Specular Reflection Sheen (upper-left curved glass sweep!)
    sheen = Image.new("RGBA", (gw, gh), (0, 0, 0, 0))
    sdr = ImageDraw.Draw(sheen)
    # Soft curved glass reflection band across top-left
    poly = [
        (0, 0),
        (int(gw * 0.64), 0),
        (int(gw * 0.34), int(gh * 0.56)),
        (0, int(gh * 0.72)),
    ]
    sdr.polygon(poly, fill=(225, 245, 240, 16))
    # Subtle thin diagonal specular glint
    glint = [
        (int(gw * 0.06), 0),
        (int(gw * 0.16), 0),
        (int(gw * 0.03), int(gh * 0.42)),
        (0, int(gh * 0.34)),
    ]
    sdr.polygon(glint, fill=(240, 255, 250, 24))
    sheen = sheen.filter(ImageFilter.GaussianBlur(radius=6))
    tube.alpha_composite(sheen)

    # Deep curved-tube inner vignette shadow around perimeter (recessed under plastic hood)
    vig = Image.new("RGBA", (gw, gh), (0, 0, 0, 0))
    vdr = ImageDraw.Draw(vig)
    vdr.rounded_rectangle([0, 0, gw - 1, gh - 1], radius=r, outline=(2, 4, 5, 230), width=10)
    vdr.rounded_rectangle([3, 3, gw - 4, gh - 4], radius=max(2, r - 3), outline=(4, 8, 9, 170), width=6)
    vig = vig.filter(ImageFilter.GaussianBlur(radius=3))
    tube.alpha_composite(vig)

    # Delicate upper & lower curved-glass catchlight reflections
    tdr = ImageDraw.Draw(tube)
    tdr.rounded_rectangle([1, 1, gw - 2, gh - 2], radius=r, outline=(96, 128, 124, 130), width=2)
    tdr.line([(r, 3), (gw - r, 3)], fill=(198, 234, 228, 84), width=2)
    tdr.line([(r * 2, gh - 4), (gw - r * 2, gh - 4)], fill=(130, 180, 174, 42), width=2)

    # Mask strictly to rounded CRT tube shape
    mask = Image.new("L", (gw, gh), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, gw - 1, gh - 1], radius=r, fill=255)
    img.paste(tube, (x0, y0), mask)


def draw_arcade_panel(img, x, y, w, h, title, banner_col, dark_text=False, compact=False):
    """Authentic 1986 physical silkscreened arcade/synth panel module with coloured header & racing stripes."""
    dr = ImageDraw.Draw(img)
    x0, y0, x1, y1 = x * SS, y * SS, (x + w) * SS, (y + h) * SS
    bh = (26 if compact else 30) * SS

    # Dark matte-enamel recessed module plate with warm cream silkscreen hairline
    dr.rounded_rectangle([x0, y0, x1, y1], radius=5 * SS,
                         fill=(23, 23, 25, 205), outline=RULE_LINE, width=int(1.6 * SS))

    # Solid 1986 colour-coded silkscreen banner
    dr.rounded_rectangle([x0 + 2 * SS, y0 + 2 * SS, x1 - 2 * SS, y0 + bh], radius=4 * SS, fill=banner_col)
    dr.rectangle([x0 + 2 * SS, y0 + bh - 6 * SS, x1 - 2 * SS, y0 + bh], fill=banner_col)

    # Subtle classic 1986 diagonal arcade racing chevrons on right end of banner
    if w >= 200:
        for k in range(3):
            sx = x1 - (14 + k * 10) * SS
            stripe_col = (24, 22, 20, 55) if dark_text else (255, 248, 232, 48)
            dr.polygon([
                (sx, y0 + bh),
                (sx + 5 * SS, y0 + bh),
                (sx + 11 * SS, y0 + 2 * SS),
                (sx + 6 * SS, y0 + 2 * SS),
            ], fill=stripe_col)

    # Crisp engraved bottom shadow rule under header
    dr.line([(x0 + 2 * SS, y0 + bh), (x1 - 2 * SS, y0 + bh)], fill=(12, 12, 13, 255), width=2 * SS)

    txt_col = DARK_INK if dark_text else CREAM
    draw_text(img, title, (x + w / 2.0) * SS, (y + (14 if compact else 16)) * SS,
              13 if compact else 14, fill=txt_col, bold=True, track=0.11)


def draw_knob_scale(img, dr, cx, cy, r_knob, label, kind="pot", ticks=None,
                    start_deg=-135, end_deg=135, accent=CREAM, label_y_offset=None, compact=False):
    cx_s, cy_s = cx * SS, cy * SS
    r_in  = (r_knob + (4 if compact else 5)) * SS
    r_out = (r_knob + (9 if compact else 11)) * SS
    r_lbl = (r_knob + ((19 if compact else 25) if kind == "choice" else (17 if compact else 21))) * SS
    fsz   = 9.0 if compact else 10.0

    if ticks is not None:
        for t, txt in ticks:
            deg = start_deg + (end_deg - start_deg) * t
            rad = math.radians(deg)
            dx, dy = math.sin(rad), -math.cos(rad)
            dr.line([(cx_s + dx * r_in, cy_s + dy * r_in),
                     (cx_s + dx * r_out, cy_s + dy * r_out)],
                    fill=accent if kind == "choice" else CREAM, width=max(2, int(1.8 * SS)))
            if txt:
                draw_text(img, txt, cx_s + dx * r_lbl, cy_s + dy * r_lbl,
                          fsz, fill=CREAM if kind == "choice" else CREAM_DIM, bold=True, track=0.01)
    else:
        for i in range(11):
            t = i / 10.0
            deg = start_deg + (end_deg - start_deg) * t
            rad = math.radians(deg)
            dx, dy = math.sin(rad), -math.cos(rad)
            major = (i in (0, 5, 10))
            ro = r_out + (3 * SS if major else 0)
            col = ARC_RED_HI if (kind == "bipolar" and i == 5) else (CREAM if major else CREAM_DIM)
            dr.line([(cx_s + dx * r_in, cy_s + dy * r_in),
                     (cx_s + dx * ro, cy_s + dy * ro)],
                    fill=col, width=max(2, int((1.9 if major else 1.2) * SS)))
        if not compact:
            lbls = [(0.0, "-"), (0.5, "0"), (1.0, "+")] if kind == "bipolar" else [(0.0, "0"), (0.5, "5"), (1.0, "10")]
            for t, txt in lbls:
                deg = start_deg + (end_deg - start_deg) * t
                rad = math.radians(deg)
                draw_text(img, txt, cx_s + math.sin(rad) * r_lbl, cy_s - math.cos(rad) * r_lbl, 9.5, fill=CREAM_DIM)

    ly_off = label_y_offset if label_y_offset is not None else (r_knob + (21 if compact else 27))
    draw_text(img, label, cx_s, (cy + ly_off) * SS, 10.5 if compact else 11.8, fill=CREAM, bold=True, track=0.06)


def draw_fader_scale(img, dr, cx, cy, fw, fh, label, top_lbl="10", bot_lbl="0",
                     bipolar=False, accent=CREAM):
    """Authentic 1986 horizontal calibrated ticks flanking a recessed vertical arcade synth fader."""
    cx_s, cy_s = cx * SS, cy * SS
    travel_top = (cy - fh * 0.36) * SS
    travel_bot = (cy + fh * 0.36) * SS
    x_in_l  = (cx - fw * 0.44) * SS
    x_out_l = (cx - fw * 0.65) * SS
    x_in_r  = (cx + fw * 0.44) * SS
    x_out_r = (cx + fw * 0.65) * SS

    for i in range(11):
        t = i / 10.0
        yy = travel_bot - t * (travel_bot - travel_top)
        major = (i in (0, 5, 10))
        ext = 4 * SS if major else 0
        col = accent if (major and ((bipolar and i == 5) or (not bipolar and i == 10))) else (CREAM if major else CREAM_DIM)
        w_ln = max(2, int((2.0 if major else 1.2) * SS))
        dr.line([(x_out_l - ext, yy), (x_in_l, yy)], fill=col, width=w_ln)
        dr.line([(x_in_r, yy), (x_out_r + ext, yy)], fill=col, width=w_ln)

    if top_lbl:
        draw_text(img, top_lbl, cx_s, (cy - fh * 0.5 - 11) * SS, 9.5,
                  fill=accent if accent != CREAM else CREAM_DIM, bold=True, track=0.04)
    if bot_lbl:
        draw_text(img, bot_lbl, cx_s, (cy + fh * 0.5 + 10) * SS, 9.5, fill=CREAM_DIM, bold=True, track=0.04)

    draw_text(img, label, cx_s, (cy + fh * 0.5 + 28) * SS, 11.8, fill=CREAM, bold=True, track=0.06)


# =============================================================================
# 1. PAGE 1 — OP3000 (1986 Arcade Deck + Curved-Glass CRT 8-Alg Display + Knobs & Faders)
# =============================================================================
def build_page1():
    img = make_op3000_chassis(seed=19861)
    dr = ImageDraw.Draw(img)

    # Top Left: OP3000 1986 Marquee Nameplate + Recessed Curved-Glass Preset VFD Readout
    draw_op3000_marquee(img, dr, 46, 22, "YM2151 OPM + YM2612 OPN2 SOUND BOARD", width=308)
    draw_realistic_crt(img, 46, 77, 308, 39, radius=6, hood=4)

    # Top Center HERO: Sculpted Curved-Glass Phosphor CRT 8-Algorithm FM Display!
    draw_realistic_crt(img, 372, 18, 496, 98, radius=10, hood=6)

    # Top Right: Dual-Bay Curved-Glass VFD Silicon Status + Stereo L/R Peak Meter Tube!
    draw_realistic_crt(img, 886, 18, 340, 98, radius=10, hood=6, divider_x=1132)
    draw_text(img, "SILICON MONITOR", 1009 * SS, 33 * SS, 9.8, fill=(116, 182, 174, 255), bold=True, track=0.11)
    dr.line([(902 * SS, 44 * SS), (1116 * SS, 44 * SS)], fill=(54, 92, 88, 150), width=2)
    draw_text(img, "44.1kHz  •  STEREO OUT", 1009 * SS, 102 * SS, 8.2, fill=(92, 148, 140, 210), bold=True, track=0.08)
    draw_text(img, "L", 1158 * SS, 105 * SS, 9.5, fill=CREAM_DIM, bold=True)
    draw_text(img, "R", 1188 * SS, 105 * SS, 9.5, fill=CREAM_DIM, bold=True)
    # Printed dB calibration legend next to VFD stereo meters
    for db_y, db_txt in [(32, "0"), (50, "-6"), (68, "-12"), (86, "-24")]:
        draw_text(img, db_txt, 1212 * SS, db_y * SS, 8.2, fill=(116, 156, 148, 230), bold=True, track=0.0)

    # 3 Colour-Banded 1986 Arcade Control Panel Modules
    draw_arcade_panel(img, 36,  136, 294, 468, "CHIP & VOICE",    ARC_TEAL)
    draw_arcade_panel(img, 342, 136, 596, 468, "FM SCULPT & LFO", ARC_RED)
    draw_arcade_panel(img, 950, 136, 294, 468, "PSG & SPACE FX",  ARC_GOLD, dark_text=True)

    # Vintage signal-flow arrowheads between module header banners
    for ax in (336, 944):
        dr.polygon([(ax * SS - 3 * SS, 146 * SS), (ax * SS + 4 * SS, 151 * SS), (ax * SS - 3 * SS, 156 * SS)],
                   fill=CREAM_DIM)

    y_top   = 258
    y_bot_k = 492
    y_bot_f = 472

    def choice_ticks(labels):
        n = len(labels)
        return [(i / float(max(1, n - 1)), l) for i, l in enumerate(labels)]

    knobs_spec = [
        # Module 1: CHIP & VOICE (4 rotary selectors — tactile physical switches!)
        (1,  "chip_mode",  109, y_top,   66, "knob_op3_teal_3way.png", 3,  "CHIP MODE", "choice",
         choice_ticks(["OPM", "OPN2", "HYB"]), -60, 60, ARC_TEAL),
        (2,  "alg",        257, y_top,   66, "knob_op3_gold_8way.png", 8,  "ALGORITHM", "choice",
         choice_ticks(["0", "1", "2", "3", "4", "5", "6", "7"]), -125, 125, ARC_GOLD),
        (3,  "feedback",   109, y_bot_k, 66, "knob_op3_gold_8way.png", 8,  "FEEDBACK",  "choice",
         choice_ticks(["0", "1", "2", "3", "4", "5", "6", "7"]), -125, 125, ARC_GOLD),
        (4,  "voice_mode", 257, y_bot_k, 66, "knob_op3_teal_3way.png", 3,  "VOICE",     "choice",
         choice_ticks(["POLY", "MONO", "UNI2"]), -60, 60, ARC_TEAL),

        # Module 2 Top Row: Hero Red FM BITE + LFO section
        (5,  "fm_bite",    416, y_top,   76, "knob_op3_red_hero.png",  65, "FM BITE",   "bipolar", None, -135, 135, ARC_RED_HI),
        (10, "lfo_speed",  565, y_top,   66, "knob_op3_silver.png",    65, "LFO SPEED", "pot",     None, -135, 135, CREAM),
        (11, "lfo_pm",     714, y_top,   66, "knob_op3_silver.png",    65, "VIB / PM",  "pot",     None, -135, 135, CREAM),
        (12, "lfo_am",     863, y_top,   66, "knob_op3_silver.png",    65, "TREM / AM", "pot",     None, -135, 135, CREAM),

        # Module 3 Top Row: PSG SUB + PCB TONE
        (13, "psg_mix",    1023, y_top,  66, "knob_op3_teal_pot.png",  65, "PSG SUB",   "pot",     None, -135, 135, ARC_TEAL),
        (14, "pcb_tone",   1171, y_top,  66, "knob_op3_gold_3way.png", 3,  "PCB TONE",  "choice",
         choice_ticks(["CRISP", "MD1", "LO-FI"]), -60, 60, ARC_GOLD),
    ]

    # Bottom Row Vertical Arcade Faders (Envelope + Detune + Chorus + Delay!)
    faders_spec = [
        (6,  "macro_att",  416,  y_bot_f, 44, 132, "fader_op3_red.png",   65, "ATTACK",    "SLOW", "FAST", True,  ARC_RED_HI),
        (7,  "macro_dec",  565,  y_bot_f, 44, 132, "fader_op3_gold.png",  65, "DECAY",     "LONG", "SNAP", True,  ARC_GOLD),
        (8,  "macro_rel",  714,  y_bot_f, 44, 132, "fader_op3_cream.png", 65, "RELEASE",   "LONG", "DRY",  True,  CREAM),
        (9,  "detune",     863,  y_bot_f, 44, 132, "fader_op3_teal.png",  65, "DETUNE",    "WIDE", "0",    False, ARC_TEAL),

        (15, "chorus_mix", 1023, y_bot_f, 44, 132, "fader_op3_teal.png",  65, "CHORUS",    "10",   "0",    False, ARC_TEAL),
        (16, "delay_mix",  1171, y_bot_f, 44, 132, "fader_op3_red.png",   65, "DELAY MIX", "10",   "0",    False, ARC_RED_HI),
    ]

    knobs = []
    for pidx, kid, cx, cy, sz, strip, frames, lbl, kind, ticks, a0, a1, acc in knobs_spec:
        draw_knob_scale(img, dr, cx, cy, sz * 0.44, lbl, kind=kind, ticks=ticks,
                        start_deg=a0, end_deg=a1, accent=acc, label_y_offset=62)
        knobs.append({
            "param": pidx, "id": kid, "cx": cx, "cy": cy, "w": sz, "h": sz,
            "strip": strip, "frames": frames, "bipolar": (kind == "bipolar"),
        })

    for pidx, kid, cx, cy, fw, fh, strip, frames, lbl, tlbl, blbl, bip, acc in faders_spec:
        draw_fader_scale(img, dr, cx, cy, fw, fh, lbl, top_lbl=tlbl, bot_lbl=blbl, bipolar=bip, accent=acc)
        knobs.append({
            "param": pidx, "id": kid, "cx": cx, "cy": cy, "w": fw, "h": fh,
            "strip": strip, "frames": frames, "bipolar": bip,
        })

    strips = [
        {"param": 2,  "id": "alg_crt", "cx": 620,  "cy": 67, "w": 476, "h": 90, "strip": "alg_crt_8.png",   "frames": 8},
        {"param": 68, "id": "meter_l", "cx": 1158, "cy": 60, "w": 22,  "h": 70, "strip": "meter_vfd_8.png", "frames": 8},
        {"param": 69, "id": "meter_r", "cx": 1188, "cy": 60, "w": 22,  "h": 70, "strip": "meter_vfd_8.png", "frames": 8},
    ]

    labels = [
        {"param": 70, "id": "stat_info", "x": 52,  "y": 80, "w": 296, "h": 33, "size": 17, "color": "ff5cf2cc"},
        {"param": 71, "id": "stat_chip", "x": 894, "y": 50, "w": 230, "h": 44, "size": 18, "color": "fffcc244"},
    ]

    out = img.resize((W, H), Image.LANCZOS).convert("RGB")
    return out, {"knobs": knobs, "strips": strips, "labels": labels}


# =============================================================================
# 2. PAGE 2 — 4-OP FM MATRIX (4 Colour-Banded Hardware Operator Plates = 32 Knobs!)
# =============================================================================
def build_page2():
    img = make_op3000_chassis(seed=19862)
    dr = ImageDraw.Draw(img)

    draw_op3000_marquee(img, dr, 46, 22, "4-OP FM OPERATOR MATRIX • DIRECT REGISTERS", width=330)

    # Curved-glass mini VFD algorithm tube + silicon status tube (clear of top-right bolt!)
    draw_realistic_crt(img, 496, 18, 366, 56, radius=8, hood=5)
    draw_realistic_crt(img, 880, 18, 346, 56, radius=8, hood=5)

    op_meta = [
        (0, "OP 1  [FB]",  ARC_GOLD,    True,  36,  "knob_op_gold_pot.png",   "knob_op_gold_16way.png"),
        (1, "OP 2",        ARC_TEAL,    False, 341, "knob_op_teal_pot.png",   "knob_op_teal_16way.png"),
        (2, "OP 3",        ARC_CREAM_B, True,  646, "knob_op_silver_pot.png", "knob_op_silver_16way.png"),
        (3, "OP 4  [OUT]", ARC_RED,     False, 951, "knob_op_red_pot.png",    "knob_op_red_16way.png"),
    ]
    rows_y = [170, 287, 404, 521]
    knobs = []

    def choice_ticks(labels):
        n = len(labels)
        return [(i / float(max(1, n - 1)), l) for i, l in enumerate(labels)]

    for op_i, title, banner, dtxt, mx, pot_strip, mul_strip in op_meta:
        draw_arcade_panel(img, mx, 88, 293, 516, title, banner, dark_text=dtxt, compact=True)
        b = 17 + op_i * 8
        cL, cR = mx + 76, mx + 217
        specs = [
            (b + 0, f"op{op_i+1}_mul", cL, rows_y[0], 54, mul_strip,            16, "RATIO",   "choice",
             [(0.0, ".5"), (4/15, "4"), (8/15, "8"), (12/15, "12"), (1.0, "15")], -140, 140),
            (b + 3, f"op{op_i+1}_lvl", cR, rows_y[0], 54, pot_strip,            65, "LEVEL",   "pot", None, -135, 135),
            (b + 1, f"op{op_i+1}_dt1", cL, rows_y[1], 54, "knob_op_7way.png",   7,  "DETUNE",  "choice",
             [(0.0, "-3"), (0.5, "0"), (1.0, "+3")], -110, 110),
            (b + 2, f"op{op_i+1}_dt2", cR, rows_y[1], 54, "knob_op_4way.png",   4,  "COARSE",  "choice",
             choice_ticks(["1.0", "1.4", "1.6", "1.7"]), -65, 65),
            (b + 4, f"op{op_i+1}_ar",  cL, rows_y[2], 54, "knob_op_silver_pot.png", 65, "ATTACK",  "pot", None, -135, 135),
            (b + 5, f"op{op_i+1}_d1r", cR, rows_y[2], 54, "knob_op_silver_pot.png", 65, "DECAY",   "pot", None, -135, 135),
            (b + 6, f"op{op_i+1}_sl",  cL, rows_y[3], 54, "knob_op_silver_pot.png", 65, "SUSTAIN", "pot", None, -135, 135),
            (b + 7, f"op{op_i+1}_rr",  cR, rows_y[3], 54, "knob_op_silver_pot.png", 65, "RELEASE", "pot", None, -135, 135),
        ]
        for pidx, kid, cx, cy, sz, strip, frames, lbl, kind, ticks, a0, a1 in specs:
            draw_knob_scale(img, dr, cx, cy, sz * 0.44, lbl, kind=kind, ticks=ticks,
                            start_deg=a0, end_deg=a1, accent=banner, label_y_offset=45, compact=True)
            knobs.append({
                "param": pidx, "id": kid, "cx": cx, "cy": cy, "w": sz, "h": sz,
                "strip": strip, "frames": frames, "bipolar": False,
            })

    strips = [
        {"param": 2, "id": "alg_crt_p2", "cx": 679, "cy": 46, "w": 348, "h": 48, "strip": "alg_crt_mini_8.png", "frames": 8},
    ]
    labels = [
        {"param": 71, "id": "stat_chip_p2", "x": 888, "y": 24, "w": 330, "h": 44, "size": 18, "color": "fffcc244"},
    ]

    out = img.resize((W, H), Image.LANCZOS).convert("RGB")
    return out, {"knobs": knobs, "strips": strips, "labels": labels}


# =============================================================================
# 3. PAGE 3 — OP3000 HYBRID / PSG / PERF & DELAY (Header reads OP3000!)
# =============================================================================
def build_page3():
    img = make_op3000_chassis(seed=19863)
    dr = ImageDraw.Draw(img)

    # Explicitly branded OP3000 (replacing "PCB & ARCADE FX" per Johnny's direction!)
    draw_op3000_marquee(img, dr, 46, 22, "HYBRID CHIP • SN76489 PSG • PERF & DELAY", width=330)

    # Dual-Bay Curved-Glass VFD Preset & Hardware Mode Monitor (clear of top-right bolt!)
    draw_realistic_crt(img, 756, 20, 470, 58, radius=8, hood=5, divider_x=906)
    draw_text(img, "OPM + OPN2", 831 * SS, 37 * SS, 10.5, fill=VFD_AMBER, bold=True, track=0.08)
    draw_text(img, "HYBRID ENGINE", 831 * SS, 57 * SS, 8.8, fill=(116, 180, 172, 235), bold=True, track=0.08)

    draw_arcade_panel(img, 36,  104, 442, 500, "DUAL-CHIP HYBRID", ARC_CREAM_B, dark_text=True)
    draw_arcade_panel(img, 490, 104, 158, 500, "PSG SUB",          ARC_TEAL)
    draw_arcade_panel(img, 660, 104, 284, 500, "PERFORMANCE",      ARC_GOLD, dark_text=True)
    draw_arcade_panel(img, 956, 104, 288, 500, "DELAY & MASTER",   ARC_RED)

    y_top   = 246
    y_bot_k = 486
    y_bot_f = 468

    def choice_ticks(labels):
        n = len(labels)
        return [(i / float(max(1, n - 1)), l) for i, l in enumerate(labels)]

    knobs_spec = [
        # DUAL-CHIP HYBRID top row (3 knobs) + bottom center (SSG-EG selector)
        (53, "chip_blend", 110, y_top,   66, "knob_op3_silver.png",     65, "CHIP BLEND", "bipolar",
         [(0.0, "OPM"), (0.5, "50"), (1.0, "OPN2")], -135, 135, ARC_TEAL),
        (54, "opn2_oct",   257, y_top,   66, "knob_op3_teal_3way.png",  3,  "OPN2 OCT",   "choice",
         choice_ticks(["-1", "0", "+1"]), -55, 55, ARC_TEAL),
        (55, "opn2_det",   404, y_top,   66, "knob_op3_silver.png",     65, "OPN2 DET",   "bipolar", None, -135, 135, CREAM),
        (57, "ssg_eg",     257, y_bot_k, 66, "knob_op3_gold_4way.png",  4,  "SSG-EG",     "choice",
         choice_ticks(["OFF", "LOOP", "ALT", "INV"]), -65, 65, ARC_GOLD),

        # PSG SUB top row
        (59, "psg_wave",   569, y_top,   66, "knob_op3_teal_5way.png",  5,  "PSG WAVE",   "choice",
         choice_ticks(["SQR", "-1", "-2", "PLS", "NOI"]), -95, 95, ARC_TEAL),

        # PERFORMANCE (2x2 knobs)
        (61, "vel_sens",   732, y_top,   66, "knob_op3_silver.png",     65, "VELOCITY",   "pot", None, -135, 135, CREAM),
        (62, "porta",      872, y_top,   66, "knob_op3_silver.png",     65, "GLIDE",      "pot", None, -135, 135, CREAM),
        (63, "bend_range", 732, y_bot_k, 66, "knob_op3_gold_13way.png", 13, "BEND RNG",   "choice",
         [(0.0, "0"), (2/12, "2"), (5/12, "5"), (7/12, "7"), (1.0, "12")], -140, 140, ARC_GOLD),
        (64, "lfo_wave",   872, y_bot_k, 66, "knob_op3_gold_4way.png",  4,  "LFO WAVE",   "choice",
         choice_ticks(["SAW", "SQR", "TRI", "RND"]), -65, 65, ARC_GOLD),

        # DELAY & MASTER top row
        (65, "delay_div",  1028, y_top,  66, "knob_op3_gold_6way.png",  6,  "DLY TIME",   "choice",
         choice_ticks(["1/16", "8T", "1/8", "8D", "1/4", "1/2"]), -115, 115, ARC_GOLD),
        (66, "delay_fb",   1172, y_top,  66, "knob_op3_silver.png",     65, "DLY FBK",    "pot", None, -135, 135, CREAM),
    ]

    # Bottom row tactile vertical faders on Page 3!
    faders_spec = [
        (56, "opm_noise",  110,  y_bot_f, 44, 132, "fader_op3_gold.png",   65, "OPM NOISE",  "31",   "OFF", False, ARC_GOLD),
        (58, "dac_grit",   404,  y_bot_f, 44, 132, "fader_op3_red.png",    65, "DAC GRIT",   "GRIT", "CLN", False, ARC_RED_HI),
        (60, "psg_decay",  569,  y_bot_f, 44, 132, "fader_op3_teal.png",   65, "PSG DECAY",  "HOLD", "PERC", False, ARC_TEAL),
        (67, "delay_tone", 1028, y_bot_f, 44, 132, "fader_op3_cream.png",  65, "DLY TONE",   "BRT",  "DARK", False, CREAM),
        (0,  "volume",     1172, y_bot_f, 52, 138, "fader_op3_master.png", 65, "MASTER VOL", "MAX",  "0",    False, ARC_RED_HI),
    ]

    knobs = []
    for pidx, kid, cx, cy, sz, strip, frames, lbl, kind, ticks, a0, a1, acc in knobs_spec:
        draw_knob_scale(img, dr, cx, cy, sz * 0.44, lbl, kind=kind, ticks=ticks,
                        start_deg=a0, end_deg=a1, accent=acc, label_y_offset=62)
        knobs.append({
            "param": pidx, "id": kid, "cx": cx, "cy": cy, "w": sz, "h": sz,
            "strip": strip, "frames": frames, "bipolar": (kind == "bipolar"),
        })

    for pidx, kid, cx, cy, fw, fh, strip, frames, lbl, tlbl, blbl, bip, acc in faders_spec:
        draw_fader_scale(img, dr, cx, cy, fw, fh, lbl, top_lbl=tlbl, bot_lbl=blbl, bipolar=bip, accent=acc)
        knobs.append({
            "param": pidx, "id": kid, "cx": cx, "cy": cy, "w": fw, "h": fh,
            "strip": strip, "frames": frames, "bipolar": bip,
        })

    labels = [
        {"param": 70, "id": "stat_info_p3", "x": 914, "y": 26, "w": 304, "h": 46, "size": 18, "color": "ff5cf2cc"},
    ]
    out = img.resize((W, H), Image.LANCZOS).convert("RGB")
    return out, {"knobs": knobs, "strips": [], "labels": labels}


# =============================================================================
# 4. PAGE 4 — FILTER (Per-Voice Hybrid VCF + Dedicated Pre-HPF + Poly ADSR)
# =============================================================================
def build_page4():
    img = make_op3000_chassis(seed=19864)
    dr = ImageDraw.Draw(img)

    # Same 1986 cabinet header & curved-glass VFD/CRT layout as Page 1!
    draw_op3000_marquee(img, dr, 46, 22, "PER-VOICE HYBRID ANALOG FILTER • LP / BP / HP", width=308)
    draw_realistic_crt(img, 46, 77, 308, 39, radius=6, hood=4)

    # Top Center HERO: Live VFD Filter Mode Response CRT (LP12 / LP24 / BP12 / HP12)
    draw_realistic_crt(img, 372, 18, 496, 98, radius=10, hood=6)

    # Top Right: Dual-Bay Curved-Glass VFD Filter Monitor + Stereo L/R Peak Meter Tube!
    draw_realistic_crt(img, 886, 18, 340, 98, radius=10, hood=6, divider_x=1132)
    draw_text(img, "VCF MONITOR", 1009 * SS, 33 * SS, 9.8, fill=(116, 182, 174, 255), bold=True, track=0.11)
    dr.line([(902 * SS, 44 * SS), (1116 * SS, 44 * SS)], fill=(54, 92, 88, 150), width=2)
    draw_text(img, "PER-VOICE ZDF-SVF + HPF", 1009 * SS, 102 * SS, 8.2, fill=(92, 148, 140, 210), bold=True, track=0.07)
    draw_text(img, "L", 1158 * SS, 105 * SS, 9.5, fill=CREAM_DIM, bold=True)
    draw_text(img, "R", 1188 * SS, 105 * SS, 9.5, fill=CREAM_DIM, bold=True)
    for db_y, db_txt in [(32, "0"), (50, "-6"), (68, "-12"), (86, "-24")]:
        draw_text(img, db_txt, 1212 * SS, db_y * SS, 8.2, fill=(116, 156, 148, 230), bold=True, track=0.0)

    # 3 Colour-Banded 1986 Arcade Control Panel Modules (exact Page 1 rhythm!)
    draw_arcade_panel(img, 36,  136, 294, 468, "ANALOG VCF",         ARC_RED)
    draw_arcade_panel(img, 342, 136, 596, 468, "FILTER ENV & DRIVE", ARC_GOLD, dark_text=True)
    draw_arcade_panel(img, 950, 136, 294, 468, "MOD & OUTPUT",       ARC_TEAL)

    for ax in (336, 944):
        dr.polygon([(ax * SS - 3 * SS, 146 * SS), (ax * SS + 4 * SS, 151 * SS), (ax * SS - 3 * SS, 156 * SS)],
                   fill=CREAM_DIM)

    y_top   = 258
    y_bot_k = 492
    y_bot_f = 472

    def choice_ticks(labels):
        n = len(labels)
        return [(i / float(max(1, n - 1)), l) for i, l in enumerate(labels)]

    cutoff_ticks = [
        (i / 10.0, "25" if i == 0 else ("800" if i == 5 else ("19k" if i == 10 else "")))
        for i in range(11)
    ]

    knobs_spec = [
        # Module 1: ANALOG VCF top row + MODE selector
        (73, "filt_cut",     109, y_top,   76, "knob_op3_red_hero.png",  65, "CUTOFF",     "pot",
         cutoff_ticks, -135, 135, ARC_RED_HI),
        (74, "filt_res",     257, y_top,   66, "knob_op3_gold_pot.png",  65, "RESO",       "pot",
         None, -135, 135, ARC_GOLD),
        (72, "filt_mode",    109, y_bot_k, 66, "knob_op3_gold_4way.png", 4,  "MODE",       "choice",
         choice_ticks(["LP12", "LP24", "BP12", "HP12"]), -65, 65, ARC_GOLD),

        # Module 2: FILTER ENV & DRIVE top row
        (78, "filt_env_amt", 416, y_top,   66, "knob_op3_silver.png",    65, "ENV DEPTH",  "bipolar", None, -135, 135, CREAM),
        (79, "filt_vel",     565, y_top,   66, "knob_op3_silver.png",    65, "VEL -> ENV", "pot",     None, -135, 135, CREAM),
        (77, "filt_key",     714, y_top,   66, "knob_op3_silver.png",    65, "KEYTRACK",   "pot",     None, -135, 135, CREAM),
        (76, "filt_drive",   863, y_top,   66, "knob_op3_gold_pot.png",  65, "OTA DRIVE",  "pot",     None, -135, 135, ARC_GOLD),

        # Module 3: MOD & OUTPUT top row
        (84, "filt_lfo",     1023, y_top,  66, "knob_op3_teal_pot.png",  65, "LFO -> CUT", "pot",     None, -135, 135, ARC_TEAL),
        (10, "lfo_speed",    1171, y_top,  66, "knob_op3_silver.png",    65, "LFO SPEED",  "pot",     None, -135, 135, CREAM),
    ]

    faders_spec = [
        # Module 1: Dedicated Pre-HPF Fader
        (75, "filt_hpf",   257,  y_bot_f, 44, 132, "fader_op3_teal.png",   65, "HPF CUT",    "4.5k", "OFF", False, ARC_TEAL),

        # Module 2: Poly Filter ADSR Vertical Faders
        (80, "filt_att",   416,  y_bot_f, 44, 132, "fader_op3_red.png",    65, "ATTACK",     "3.5s", "1ms", False, ARC_RED_HI),
        (81, "filt_dec",   565,  y_bot_f, 44, 132, "fader_op3_gold.png",   65, "DECAY",      "6.0s", "8ms", False, ARC_GOLD),
        (82, "filt_sus",   714,  y_bot_f, 44, 132, "fader_op3_cream.png",  65, "SUSTAIN",    "10",   "0",   False, CREAM),
        (83, "filt_rel",   863,  y_bot_f, 44, 132, "fader_op3_teal.png",   65, "RELEASE",    "6.0s", "8ms", False, ARC_TEAL),

        # Module 3: FM Bite & Master Output Faders
        (5,  "fm_bite",    1023, y_bot_f, 44, 132, "fader_op3_gold.png",   65, "FM BITE",    "+",    "-",   True,  ARC_GOLD),
        (0,  "volume",     1171, y_bot_f, 52, 138, "fader_op3_master.png", 65, "MASTER VOL", "MAX",  "0",   False, ARC_RED_HI),
    ]

    knobs = []
    for pidx, kid, cx, cy, sz, strip, frames, lbl, kind, ticks, a0, a1, acc in knobs_spec:
        draw_knob_scale(img, dr, cx, cy, sz * 0.44, lbl, kind=kind, ticks=ticks,
                        start_deg=a0, end_deg=a1, accent=acc, label_y_offset=62)
        knobs.append({
            "param": pidx, "id": kid, "cx": cx, "cy": cy, "w": sz, "h": sz,
            "strip": strip, "frames": frames, "bipolar": (kind == "bipolar"),
        })

    for pidx, kid, cx, cy, fw, fh, strip, frames, lbl, tlbl, blbl, bip, acc in faders_spec:
        draw_fader_scale(img, dr, cx, cy, fw, fh, lbl, top_lbl=tlbl, bot_lbl=blbl, bipolar=bip, accent=acc)
        knobs.append({
            "param": pidx, "id": kid, "cx": cx, "cy": cy, "w": fw, "h": fh,
            "strip": strip, "frames": frames, "bipolar": bip,
        })

    strips = [
        {"param": 72, "id": "filt_crt",   "cx": 620,  "cy": 67, "w": 476, "h": 90, "strip": "filt_crt_4.png",  "frames": 4},
        {"param": 68, "id": "meter_l_p4", "cx": 1158, "cy": 60, "w": 22,  "h": 70, "strip": "meter_vfd_8.png", "frames": 8},
        {"param": 69, "id": "meter_r_p4", "cx": 1188, "cy": 60, "w": 22,  "h": 70, "strip": "meter_vfd_8.png", "frames": 8},
    ]

    labels = [
        {"param": 70, "id": "stat_info_p4", "x": 52,  "y": 80, "w": 296, "h": 33, "size": 17, "color": "ff5cf2cc"},
        {"param": 85, "id": "stat_filt_p4", "x": 894, "y": 50, "w": 230, "h": 44, "size": 18, "color": "fffcc244"},
    ]

    out = img.resize((W, H), Image.LANCZOS).convert("RGB")
    return out, {"knobs": knobs, "strips": strips, "labels": labels}


# =============================================================================
# 4B. HERO 4-FRAME GLOWING VFD / PHOSPHOR CRT BODE FILTER CURVE STRIP!
# =============================================================================
def render_filt_frame(mode, w, h):
    """Draw one glowing 1986 VFD/CRT Bode magnitude plot for Hybrid Filter mode 0..3."""
    s = 2
    sw, sh = w * s, h * s
    im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    glow = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    dr = ImageDraw.Draw(im)
    gdr = ImageDraw.Draw(glow)

    meta = {
        0: ("LP12", "12dB LOWPASS",   VFD_MINT),
        1: ("LP24", "24dB LADDER LP", VFD_MINT),
        2: ("BP12", "12dB BANDPASS",  VFD_AMBER),
        3: ("HP12", "12dB HIGHPASS",  VFD_CORAL),
    }
    tag, desc, col = meta[mode]

    # Left VFD readout labels
    font_big = load_font(int(sh * 0.25), bold=True)
    font_sm  = load_font(int(sh * 0.12), bold=True)
    gdr.text((int(sw * 0.03), int(sh * 0.14)), tag,  font=font_big, fill=(col[0], col[1], col[2], 140))
    dr.text((int(sw * 0.03),  int(sh * 0.14)), tag,  font=font_big, fill=col)
    dr.text((int(sw * 0.03),  int(sh * 0.50)), desc, font=font_sm,  fill=VFD_AMBER)
    dr.text((int(sw * 0.03),  int(sh * 0.72)), "+ PRE-HPF  •  TPT OTA", font=font_sm, fill=(132, 182, 174, 240))

    # Right VFD oscilloscope Bode graph box
    gx0, gy0 = int(sw * 0.39), int(sh * 0.14)
    gx1, gy1 = int(sw * 0.96), int(sh * 0.88)
    gw, gh = gx1 - gx0, gy1 - gy0
    y_0db = gy0 + gh * 0.38

    # Subtle phosphor graticule lines (0 dB reference + log frequency ticks)
    for frac in (0.12, 0.38, 0.64, 0.90):
        yy = int(gy0 + gh * frac)
        dr.line([(gx0, yy), (gx1, yy)], fill=(48, 88, 84, 90 if abs(frac - 0.38) > 0.01 else 150), width=1)
    for i in range(1, 6):
        xx = int(gx0 + gw * (i / 6.0))
        dr.line([(xx, gy0), (xx, gy1)], fill=(48, 88, 84, 80), width=1)
    dr.rectangle([gx0, gy0, gx1, gy1], outline=(76, 128, 122, 160), width=2)

    # Pre-HPF subtle amber dash-hint on bottom-left (2-pole Butterworth rolloff below u_hpf)
    hpf_pts = []
    u_hpf = 0.22
    for px in range(0, gw + 1, 4):
        u = px / float(gw)
        r = (2.0 ** ((u - u_hpf) * 8.5))
        r2 = r * r
        mag_hpf = r2 / math.sqrt((1.0 - r2) ** 2 + 2.0 * r2)
        db_hpf = max(-28.0, min(6.0, 20.0 * math.log10(max(1e-4, mag_hpf))))
        yy = y_0db - db_hpf * (gh * 0.022)
        yy = max(gy0 + 3, min(gy1 - 3, yy))
        hpf_pts.append((gx0 + px, yy))
    for k in range(0, len(hpf_pts) - 1, 2):
        dr.line([hpf_pts[k], hpf_pts[k + 1]], fill=(252, 194, 68, 130), width=2)

    # Main TPT SVF Bode curve with characteristic resonant peak around normalized cutoff
    pts = []
    u_fc = 0.50 if mode == 2 else (0.42 if mode == 3 else 0.56)
    q_damp = 0.42  # tasteful resonant peak
    for px in range(0, gw + 1, 3):
        u = px / float(gw)
        w_n = 2.0 ** ((u - u_fc) * 8.2)
        w2 = w_n * w_n
        denom1 = math.sqrt((1.0 - w2) ** 2 + (q_damp * w_n) ** 2)
        if mode == 0:      # LP12
            mag = 1.0 / denom1
        elif mode == 1:    # LP24 (cascaded)
            d1 = math.sqrt((1.0 - w2) ** 2 + (0.55 * w_n) ** 2)
            d2 = math.sqrt((1.0 - w2) ** 2 + (0.62 * w_n) ** 2)
            mag = 1.0 / (d1 * d2)
        elif mode == 2:    # BP12
            mag = (0.72 * w_n) / denom1
        else:              # HP12
            mag = w2 / denom1
        db = max(-26.0, min(9.5, 20.0 * math.log10(max(1e-4, mag))))
        yy = y_0db - db * (gh * 0.024)
        yy = max(gy0 + 4, min(gy1 - 4, yy))
        pts.append((gx0 + px, yy))

    if len(pts) >= 2:
        gdr.line(pts, fill=(col[0], col[1], col[2], 180), width=8)
        dr.line(pts,  fill=col, width=4)
        # Subtle glowing cutoff marker dot on peak
        fc_idx = min(len(pts) - 1, int(u_fc * (len(pts) - 1)))
        fx, fy = pts[fc_idx]
        gdr.ellipse([fx - 7, fy - 7, fx + 7, fy + 7], fill=(col[0], col[1], col[2], 200))
        dr.ellipse([fx - 4, fy - 4, fx + 4, fy + 4], fill=(250, 255, 252, 255))

    glow = glow.filter(ImageFilter.GaussianBlur(radius=5))
    comp = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    comp.alpha_composite(glow)
    comp.alpha_composite(im)
    return comp.resize((w, h), Image.LANCZOS)


def make_filt_crt_strip(w=476, h=90):
    strip = Image.new("RGBA", (w, h * 4), (0, 0, 0, 0))
    for mode in range(4):
        strip.paste(render_filt_frame(mode, w, h), (0, mode * h))
    return strip


# =============================================================================
# 4. HERO 8-FRAME GLOWING VFD / PHOSPHOR CRT ALGORITHM SCHEMATIC!
# =============================================================================
def render_alg_frame(alg, w, h, mini=False):
    """Draw one glowing 1986 VFD/CRT phosphor schematic for Yamaha 4-op FM Algorithm 0..7."""
    s = 2
    sw, sh = w * s, h * s
    im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    glow = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    dr = ImageDraw.Draw(im)
    gdr = ImageDraw.Draw(glow)

    layouts = {
        0: ([(0.18, 0.54), (0.39, 0.54), (0.60, 0.54), (0.82, 0.54)], [(0,1),(1,2),(2,3)], "SERIAL 4-STACK"),
        1: ([(0.20, 0.30), (0.20, 0.76), (0.52, 0.54), (0.82, 0.54)], [(0,2),(1,2),(2,3)], "DUAL MOD STACK"),
        2: ([(0.22, 0.28), (0.22, 0.76), (0.52, 0.76), (0.82, 0.54)], [(0,3),(1,2),(2,3)], "SPLIT MOD OUT"),
        3: ([(0.20, 0.30), (0.50, 0.30), (0.50, 0.76), (0.82, 0.54)], [(0,1),(1,3),(2,3)], "CASCADE + MOD"),
        4: ([(0.26, 0.30), (0.72, 0.30), (0.26, 0.76), (0.72, 0.76)], [(0,1),(2,3)],       "DUAL 2-OP PAIRS"),
        5: ([(0.22, 0.54), (0.72, 0.22), (0.72, 0.54), (0.72, 0.86)], [(0,1),(0,2),(0,3)], "1 MOD 3 CARRIER"),
        6: ([(0.22, 0.26), (0.72, 0.26), (0.72, 0.56), (0.72, 0.86)], [(0,1)],             "1 PAIR + 2 SINE"),
        7: ([(0.20, 0.54), (0.40, 0.54), (0.60, 0.54), (0.80, 0.54)], [],                  "4-OP ADDITIVE"),
    }
    coords, edges, desc = layouts[alg]
    carriers = [alg == 7, alg >= 4, alg >= 5, True]

    bw = int(sw * (0.095 if mini else 0.088))
    bh = int(sh * (0.35  if mini else 0.26))

    def px_pos(nx, ny):
        x_lo = sw * (0.26 if mini else 0.22)
        x_hi = sw * 0.90
        y_lo = sh * 0.14
        y_hi = sh * 0.88
        return (x_lo + nx * (x_hi - x_lo), y_lo + ny * (y_hi - y_lo))

    pts = [px_pos(nx, ny) for nx, ny in coords]

    # Left glowing VFD phosphor title readout
    font_big = load_font(int(sh * (0.35 if mini else 0.25)), bold=True)
    font_sm  = load_font(int(sh * (0.18 if mini else 0.12)), bold=True)
    gdr.text((int(sw * 0.03), int(sh * (0.14 if mini else 0.14))), f"ALG {alg}", font=font_big, fill=(92, 242, 204, 140))
    dr.text((int(sw * 0.03),  int(sh * (0.14 if mini else 0.14))), f"ALG {alg}", font=font_big, fill=VFD_MINT)
    if not mini:
        dr.text((int(sw * 0.03), int(sh * 0.50)), desc, font=font_sm, fill=VFD_AMBER)
        dr.text((int(sw * 0.03), int(sh * 0.72)), "MINT=MOD  RED=OUT", font=font_sm, fill=(132, 182, 174, 240))

    # Glowing VFD trace paths between operators
    for a, b in edges:
        x0, y0 = pts[a]
        x1, y1 = pts[b]
        p0 = (x0 + bw * 0.52, y0)
        p1 = (x1 - bw * 0.52, y1)
        gdr.line([p0, p1], fill=(92, 242, 204, 150), width=7)
        dr.line([p0, p1], fill=VFD_MINT, width=3)

    # Amber-phosphor Feedback loop arc above OP 1
    fx, fy = pts[0]
    fb_top = bh * (0.80 if mini else 0.94)
    fb_r = [fx - bw * 0.65, fy - fb_top, fx + bw * 0.65, fy - bh * 0.10]
    gdr.arc(fb_r, 180, 360, fill=(252, 194, 68, 165), width=6)
    dr.arc(fb_r, 180, 360, fill=VFD_AMBER, width=3)

    # Coral-red glowing OUT bus on far right for all Carriers
    out_x = int(sw * 0.94)
    car_ys = []
    for idx, is_c in enumerate(carriers):
        if is_c:
            cx, cy = pts[idx]
            car_ys.append(cy)
            gdr.line([(cx + bw * 0.52, cy), (out_x, cy)], fill=(248, 96, 76, 160), width=7)
            dr.line([(cx + bw * 0.52, cy), (out_x, cy)], fill=VFD_CORAL, width=3)
    if len(car_ys) > 1:
        gdr.line([(out_x, min(car_ys)), (out_x, max(car_ys))], fill=(248, 96, 76, 160), width=8)
        dr.line([(out_x, min(car_ys)), (out_x, max(car_ys))], fill=VFD_CORAL, width=4)

    # Glowing VFD Operator Cells
    font_op = load_font(int(bh * 0.72), bold=True)
    for idx, (cx, cy) in enumerate(pts):
        is_c = carriers[idx]
        col = VFD_CORAL if is_c else VFD_MINT
        fill_c = (56, 20, 18, 240) if is_c else (12, 46, 42, 240)
        box = [cx - bw / 2, cy - bh / 2, cx + bw / 2, cy + bh / 2]
        gdr.rounded_rectangle([box[0] - 3, box[1] - 3, box[2] + 3, box[3] + 3], radius=8, outline=col, width=5)
        dr.rounded_rectangle(box, radius=6, fill=fill_c, outline=VFD_AMBER if idx == 0 else col, width=3)
        lbl = str(idx + 1)
        bb = dr.textbbox((0, 0), lbl, font=font_op)
        dr.text((cx - (bb[2] - bb[0]) / 2, cy - (bb[3] - bb[1]) / 2 - 2), lbl, font=font_op, fill=(246, 252, 248, 255))

    glow = glow.filter(ImageFilter.GaussianBlur(radius=5))
    comp = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    comp.alpha_composite(glow)
    comp.alpha_composite(im)
    return comp.resize((w, h), Image.LANCZOS)


def make_alg_crt_strip(w, h, mini=False):
    strip = Image.new("RGBA", (w, h * 8), (0, 0, 0, 0))
    for alg in range(8):
        strip.paste(render_alg_frame(alg, w, h, mini=mini), (0, alg * h))
    return strip


# =============================================================================
# 5. 4X-SUPERSAMPLED 1986 COLOURED-CAP ARCADE KNOBS, VERTICAL FADERS & VFD METERS
# =============================================================================
def render_op3000_knob(size, deg, cap_col=(168, 164, 156), dark_ptr_on_cap=True):
    """Authentic 1986 Japanese synth/arcade 12-ribbed fluted skirt + glossy coloured aluminum top cap."""
    s = size * 4
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    dr = ImageDraw.Draw(im)
    cx = cy = s / 2.0
    r_skirt = s * 0.44
    r_body  = s * 0.34
    r_cap   = s * 0.24

    # Soft studio drop shadow
    sh = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    sdr = ImageDraw.Draw(sh)
    sdr.ellipse([cx - r_skirt + s * 0.025, cy - r_skirt + s * 0.045,
                 cx + r_skirt + s * 0.025, cy + r_skirt + s * 0.045], fill=(0, 0, 0, 175))
    sh = sh.filter(ImageFilter.GaussianBlur(radius=s * 0.032))
    im.alpha_composite(sh)

    # Dark charcoal Bakelite outer skirt
    dr.ellipse([cx - r_skirt, cy - r_skirt, cx + r_skirt, cy + r_skirt],
               fill=(22, 22, 24, 255), outline=(62, 60, 58, 255), width=max(2, s // 60))

    # 12 physical molded skirt flutes
    rad = math.radians(deg)
    ribs = 12
    for i in range(ribs):
        a = rad + (2.0 * math.pi * i) / ribs
        lx = cx + math.sin(a) * (r_skirt * 0.88)
        ly = cy - math.cos(a) * (r_skirt * 0.88)
        lr = r_skirt * 0.115
        shade = 36 + int(19 * (-math.sin(a) * 0.55 + math.cos(a) * 0.82))
        dr.ellipse([lx - lr, ly - lr, lx + lr, ly + lr], fill=(shade, shade, shade + 1, 255))

    # Tapered charcoal knob cone
    dr.ellipse([cx - r_body, cy - r_body, cx + r_body, cy + r_body],
               fill=(28, 28, 30, 255), outline=(72, 70, 68, 255), width=max(2, s // 72))

    # Glossy anodized coloured aluminum top cap!
    cr, cg, cb = cap_col
    cap_rim = (min(255, cr + 48), min(255, cg + 48), min(255, cb + 48), 255)
    cap_in  = (max(0, cr - 28),   max(0, cg - 28),   max(0, cb - 28),   170)
    cap_hi  = (min(255, cr + 88), min(255, cg + 88), min(255, cb + 88), 220)

    dr.ellipse([cx - r_cap, cy - r_cap, cx + r_cap, cy + r_cap],
               fill=(cr, cg, cb, 255), outline=cap_rim, width=max(2, s // 70))
    dr.ellipse([cx - r_cap * 0.72, cy - r_cap * 0.72, cx + r_cap * 0.72, cy + r_cap * 0.72],
               outline=cap_in, width=max(2, s // 92))
    dr.arc([cx - r_cap + 3, cy - r_cap + 3, cx + r_cap - 3, cy + r_cap - 3],
           200, 335, fill=cap_hi, width=max(3, s // 58))

    # Crisp ivory skirt pointer notch + etched cap indicator line
    dx, dy = math.sin(rad), -math.cos(rad)
    pw = max(6, int(s * 0.050))
    p_mid = (cx + dx * (r_cap * 0.96), cy + dy * (r_cap * 0.96))
    p_out = (cx + dx * (r_skirt * 0.94), cy + dy * (r_skirt * 0.94))
    dr.line([p_mid, p_out], fill=(246, 240, 226, 255), width=pw)

    p_in = (cx + dx * (r_cap * 0.16), cy + dy * (r_cap * 0.16))
    cap_ptr = (24, 22, 20, 255) if dark_ptr_on_cap else (250, 246, 236, 255)
    dr.line([p_in, p_mid], fill=cap_ptr, width=max(5, int(s * 0.038)))

    return im.resize((size, size), Image.LANCZOS)


def make_knob_strip(size, frames, a0, a1, cap_col=(168, 164, 156), dark_ptr=True):
    strip = Image.new("RGBA", (size, size * frames), (0, 0, 0, 0))
    for i in range(frames):
        t = i / float(max(1, frames - 1))
        deg = a0 + (a1 - a0) * t
        fr = render_op3000_knob(size, deg, cap_col=cap_col, dark_ptr_on_cap=dark_ptr)
        strip.paste(fr, (0, i * size))
    assert strip.height <= 16384
    return strip


def make_op3000_fader_strip(fw, fh, frames=65, stripe_col=CREAM):
    """Authentic 1986 recessed vertical synth/arcade fader strip (frame 0 = bottom, frame N-1 = top)."""
    strip = Image.new("RGBA", (fw, fh * frames), (0, 0, 0, 0))
    sw, sh = fw * 4, fh * 4
    slot_w = max(8, int(sw * 0.14))
    slot_top = int(sh * 0.08)
    slot_bot = int(sh * 0.92)
    cap_w = int(sw * 0.80)
    cap_h = int(sh * 0.21)
    y_min_c = int(sh * 0.14)
    y_max_c = int(sh * 0.86)

    for i in range(frames):
        t = i / float(max(1, frames - 1))
        cy = y_max_c - t * (y_max_c - y_min_c)

        im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        dr = ImageDraw.Draw(im)

        # Recessed vertical metal groove track
        sx0 = (sw - slot_w) // 2
        sx1 = sx0 + slot_w
        dr.rounded_rectangle([sx0 - 4, slot_top - 4, sx1 + 4, slot_bot + 4],
                             radius=6, fill=(46, 45, 44, 255))
        dr.rounded_rectangle([sx0, slot_top, sx1, slot_bot],
                             radius=4, fill=(8, 8, 9, 255))

        # Cast shadow under sculpted fader cap
        cx0 = (sw - cap_w) // 2
        cx1 = cx0 + cap_w
        cy0 = int(cy - cap_h / 2)
        cy1 = cy0 + cap_h
        sh_im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        sdr = ImageDraw.Draw(sh_im)
        sdr.rounded_rectangle([cx0 + 4, cy0 + 8, cx1 + 4, cy1 + 10], radius=10, fill=(0, 0, 0, 175))
        sh_im = sh_im.filter(ImageFilter.GaussianBlur(radius=8))
        im.alpha_composite(sh_im)
        dr = ImageDraw.Draw(im)

        # Sculpted 3D saddle fader cap with beveled faces & bold coloured center stripe
        dr.rounded_rectangle([cx0, cy0, cx1, cy1], radius=9,
                             fill=(34, 34, 36, 255), outline=(92, 90, 86, 255), width=3)
        dr.rounded_rectangle([cx0 + 4, cy0 + 4, cx1 - 4, cy0 + int(cap_h * 0.34)],
                             radius=5, fill=(58, 57, 55, 255))
        dr.rounded_rectangle([cx0 + 4, cy1 - int(cap_h * 0.34), cx1 - 4, cy1 - 4],
                             radius=5, fill=(18, 18, 20, 255))
        mid_y0 = int(cy - cap_h * 0.18)
        mid_y1 = int(cy + cap_h * 0.18)
        dr.rectangle([cx0 + 4, mid_y0, cx1 - 4, mid_y1], fill=(22, 22, 24, 255))
        st_h = max(6, int(cap_h * 0.15))
        dr.rectangle([cx0 + 6, int(cy - st_h / 2), cx1 - 6, int(cy + st_h / 2)], fill=stripe_col)

        strip.paste(im.resize((fw, fh), Image.LANCZOS), (0, i * fh))

    assert strip.height <= 16384
    return strip


def make_vfd_meter_strip(w=22, h=70, frames=8):
    """8-frame 1986 VFD vacuum-fluorescent peak bar meter (glowing mint -> amber -> coral red)."""
    strip = Image.new("RGBA", (w, h * frames), (0, 0, 0, 0))
    sw, sh = w * 4, h * 4
    cols = [
        (72, 232, 186),
        (82, 238, 192),
        (96, 242, 198),
        (246, 198, 66),
        (248, 162, 54),
        (246, 102, 64),
        (246, 68, 62),
    ]
    for f in range(frames):
        im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        gl = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        dr = ImageDraw.Draw(im)
        gdr = ImageDraw.Draw(gl)
        pad_x = int(sw * 0.12)
        seg_h = int(sh * 0.095)
        gap_y = int(sh * 0.042)
        top_m = int(sh * 0.05)
        for seg in range(7):
            y0 = top_m + (6 - seg) * (seg_h + gap_y)
            y1 = y0 + seg_h
            r, g, b = cols[seg]
            if seg < f:
                gdr.rounded_rectangle([pad_x - 4, y0 - 4, sw - pad_x + 4, y1 + 4], radius=5, fill=(r, g, b, 165))
                dr.rounded_rectangle([pad_x, y0, sw - pad_x, y1], radius=4, fill=(r, g, b, 255))
                dr.rounded_rectangle([pad_x + 3, y0 + 2, sw - pad_x - 3, y0 + seg_h // 2],
                                     radius=3, fill=(min(255, r + 60), min(255, g + 60), min(255, b + 60), 190))
            else:
                dr.rounded_rectangle([pad_x, y0, sw - pad_x, y1],
                                     radius=4, fill=(r // 8 + 14, g // 8 + 20, b // 8 + 18, 255))
        gl = gl.filter(ImageFilter.GaussianBlur(radius=6))
        comp = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        comp.alpha_composite(gl)
        comp.alpha_composite(im)
        strip.paste(comp.resize((w, h), Image.LANCZOS), (0, f * h))
    return strip


def main():
    os.makedirs(SKIN_DIR, exist_ok=True)
    for f in os.listdir(SKIN_DIR):
        if f.endswith(".png"):
            os.remove(os.path.join(SKIN_DIR, f))

    print("Rendering OP3000 Page 1 (1986 Arcade Cabinet + Curved-Glass CRT + Knobs & Faders)...")
    bg1, m1 = build_page1()
    bg1.save(os.path.join(SKIN_DIR, "bg_arcade.png"))

    print("Rendering OP3000 Page 2 (4-OP FM OPERATOR MATRIX)...")
    bg2, m2 = build_page2()
    bg2.save(os.path.join(SKIN_DIR, "bg_ops.png"))

    print("Rendering OP3000 Page 3 (OP3000 HYBRID / PSG / PERF & DELAY)...")
    bg3, m3 = build_page3()
    bg3.save(os.path.join(SKIN_DIR, "bg_pcb.png"))

    print("Rendering OP3000 Page 4 (PER-VOICE HYBRID ANALOG FILTER)...")
    bg4, m4 = build_page4()
    bg4.save(os.path.join(SKIN_DIR, "bg_filter.png"))

    print("Rendering VFD/CRT Diagrams, Coloured-Cap Knobs & Vertical Faders...")
    CAP_SILVER = (172, 168, 160)
    CAP_RED    = (206, 56, 48)
    CAP_GOLD   = (216, 166, 54)
    CAP_TEAL   = (54, 152, 160)

    strips = {
        "alg_crt_8.png":            make_alg_crt_strip(476, 90, mini=False),
        "alg_crt_mini_8.png":       make_alg_crt_strip(348, 48, mini=True),
        "filt_crt_4.png":           make_filt_crt_strip(476, 90),
        "meter_vfd_8.png":          make_vfd_meter_strip(22, 70, 8),

        # Page 1, 3 & 4 Coloured-Cap Knobs
        "knob_op3_silver.png":      make_knob_strip(66, 65, -135, 135, cap_col=CAP_SILVER, dark_ptr=True),
        "knob_op3_teal_pot.png":    make_knob_strip(66, 65, -135, 135, cap_col=CAP_TEAL,   dark_ptr=False),
        "knob_op3_gold_pot.png":    make_knob_strip(66, 65, -135, 135, cap_col=CAP_GOLD,   dark_ptr=True),
        "knob_op3_red_hero.png":    make_knob_strip(76, 65, -135, 135, cap_col=CAP_RED,    dark_ptr=False),
        "knob_op3_teal_3way.png":   make_knob_strip(66, 3,  -60,  60,  cap_col=CAP_TEAL,   dark_ptr=False),
        "knob_op3_teal_5way.png":   make_knob_strip(66, 5,  -95,  95,  cap_col=CAP_TEAL,   dark_ptr=False),
        "knob_op3_gold_3way.png":   make_knob_strip(66, 3,  -60,  60,  cap_col=CAP_GOLD,   dark_ptr=True),
        "knob_op3_gold_4way.png":   make_knob_strip(66, 4,  -65,  65,  cap_col=CAP_GOLD,   dark_ptr=True),
        "knob_op3_gold_6way.png":   make_knob_strip(66, 6,  -115, 115, cap_col=CAP_GOLD,   dark_ptr=True),
        "knob_op3_gold_8way.png":   make_knob_strip(66, 8,  -125, 125, cap_col=CAP_GOLD,   dark_ptr=True),
        "knob_op3_gold_13way.png":  make_knob_strip(66, 13, -140, 140, cap_col=CAP_GOLD,   dark_ptr=True),

        # Page 2 Compact 54px Colour-Coded Operator Knobs
        "knob_op_silver_pot.png":   make_knob_strip(54, 65, -135, 135, cap_col=CAP_SILVER, dark_ptr=True),
        "knob_op_gold_pot.png":     make_knob_strip(54, 65, -135, 135, cap_col=CAP_GOLD,   dark_ptr=True),
        "knob_op_teal_pot.png":     make_knob_strip(54, 65, -135, 135, cap_col=CAP_TEAL,   dark_ptr=False),
        "knob_op_red_pot.png":      make_knob_strip(54, 65, -135, 135, cap_col=CAP_RED,    dark_ptr=False),
        "knob_op_silver_16way.png": make_knob_strip(54, 16, -140, 140, cap_col=CAP_SILVER, dark_ptr=True),
        "knob_op_gold_16way.png":   make_knob_strip(54, 16, -140, 140, cap_col=CAP_GOLD,   dark_ptr=True),
        "knob_op_teal_16way.png":   make_knob_strip(54, 16, -140, 140, cap_col=CAP_TEAL,   dark_ptr=False),
        "knob_op_red_16way.png":    make_knob_strip(54, 16, -140, 140, cap_col=CAP_RED,    dark_ptr=False),
        "knob_op_4way.png":         make_knob_strip(54, 4,  -65,  65,  cap_col=CAP_SILVER, dark_ptr=True),
        "knob_op_7way.png":         make_knob_strip(54, 7,  -110, 110, cap_col=CAP_SILVER, dark_ptr=True),

        # Authentic 1986 Recessed Vertical Faders
        "fader_op3_red.png":        make_op3000_fader_strip(44, 132, 65, stripe_col=ARC_RED_HI),
        "fader_op3_gold.png":       make_op3000_fader_strip(44, 132, 65, stripe_col=ARC_GOLD),
        "fader_op3_teal.png":       make_op3000_fader_strip(44, 132, 65, stripe_col=(82, 206, 214, 255)),
        "fader_op3_cream.png":      make_op3000_fader_strip(44, 132, 65, stripe_col=CREAM),
        "fader_op3_master.png":     make_op3000_fader_strip(52, 138, 65, stripe_col=ARC_RED_HI),
    }
    for name, im in strips.items():
        im.save(os.path.join(SKIN_DIR, name))

    with open(MANIFEST, "w", encoding="utf-8") as fh:
        json.dump({"page1": m1, "page2": m2, "page3": m3, "page4": m4}, fh, indent=2)
    print(f"Done! Wrote OP3000 1986 vintage arcade skins to {SKIN_DIR}")


if __name__ == "__main__":
    main()
