#!/usr/bin/env python3
"""Render live 1280x628 visual previews of all 3 OP-ARCADE pages."""
import json
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIN_DIR = os.path.join(ROOT, "plugins", "op_arcade", "skins")
MANIFEST = os.path.join(ROOT, "tools", "ui_manifest.json")
OUT_DIR  = os.path.join(ROOT, "docs", "previews")
os.makedirs(OUT_DIR, exist_ok=True)


def get_frame(strip, frames, idx):
    im = Image.open(os.path.join(SKIN_DIR, strip)).convert("RGBA")
    fh = im.height // frames
    idx = max(0, min(frames - 1, idx))
    return im.crop((0, idx * fh, im.width, (idx + 1) * fh))


def draw_label(canvas, lb, txt):
    dr = ImageDraw.Draw(canvas)
    sz = max(11, int(round(lb["size"] * 0.72)))
    try:
        font = ImageFont.truetype("/System/Library/Fonts/HelveticaNeue.ttc", sz, index=1)
    except Exception:
        font = ImageFont.load_default()
    c = lb["color"]
    col = (int(c[2:4], 16), int(c[4:6], 16), int(c[6:8], 16), 255)
    bb = dr.textbbox((0, 0), txt, font=font)
    dr.text((lb["x"] + (lb["w"] - (bb[2] - bb[0])) // 2,
             lb["y"] + (lb["h"] - (bb[3] - bb[1])) // 2), txt, font=font, fill=col)


def main():
    with open(MANIFEST, "r", encoding="utf-8") as fh:
        m = json.load(fh)

    vals = {
        "chip_mode": 1.0, "alg": 4/7, "feedback": 6/7, "voice_mode": 0.0,
        "fm_bite": 0.62, "macro_att": 0.5, "macro_dec": 0.5, "macro_rel": 0.5,
        "detune": 0.30, "lfo_speed": 0.40, "lfo_pm": 0.15, "lfo_am": 0.0,
        "psg_mix": 0.25, "pcb_tone": 0.0, "chorus_mix": 0.38, "delay_mix": 0.24,
        "chip_blend": 0.5, "opn2_oct": 0.5, "opn2_det": 0.61, "opm_noise": 0.0,
        "ssg_eg": 0.0, "dac_grit": 0.0, "psg_wave": 0.25, "psg_decay": 0.35,
        "vel_sens": 0.55, "porta": 0.0, "bend_range": 2/12, "lfo_wave": 2/3,
        "delay_div": 3/5, "delay_fb": 0.4, "delay_tone": 0.65, "delay_send_p3": 0.24, "volume": 0.80,
        "filt_mode": 1/3, "filt_cut": 0.68, "filt_res": 0.42, "filt_hpf": 0.18,
        "filt_drive": 0.25, "filt_key": 0.50, "filt_env_amt": 0.72, "filt_vel": 0.45,
        "filt_att": 0.08, "filt_dec": 0.38, "filt_sus": 0.35, "filt_rel": 0.35,
        "filt_lfo": 0.16,
    }

    for pg, bg, out_name, lbl_txts in [
        ("page1", "bg_arcade.png", "page1_arcade.png", ["Outrun-ish Brass | HYB | V:4", "YM2151 + YM2612 • ALG 4"]),
        ("page2", "bg_ops.png",    "page2_ops.png",    ["YM2151 + YM2612 • ALG 4"]),
        ("page3", "bg_pcb.png",    "page3_pcb.png",    ["Outrun-ish Brass | HYB | V:4"]),
        ("page4", "bg_filter.png", "page4_filter.png", ["Outrun-ish Brass | HYB | V:4", "LP24 • 2.4kHz • R:42%"]),
    ]:
        im = Image.open(os.path.join(SKIN_DIR, bg)).convert("RGBA")
        for st in m[pg].get("strips", []):
            if "alg" in st["id"]:
                fi = 4
            elif "filt" in st["id"]:
                fi = 1
            elif "meter_l" in st["id"]:
                fi = 5
            else:
                fi = 6
            fr = get_frame(st["strip"], st["frames"], fi)
            im.alpha_composite(fr, (int(st["cx"] - st["w"]/2), int(st["cy"] - st["h"]/2)))
        for k in m[pg]["knobs"]:
            v = vals.get(k["id"], 0.55)
            fr = get_frame(k["strip"], k["frames"], int(round(v * (k["frames"] - 1))))
            im.alpha_composite(fr, (int(k["cx"] - k["w"]/2), int(k["cy"] - k["h"]/2)))
        for lb, txt in zip(m[pg].get("labels", []), lbl_txts):
            draw_label(im, lb, txt)
        im.convert("RGB").save(os.path.join(OUT_DIR, out_name))
    print(f"Rendered all 4 OP3000 previews to {OUT_DIR}/")


if __name__ == "__main__":
    main()
