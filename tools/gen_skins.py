#!/usr/bin/env python3
"""Generate valid Akai MPC TUI.json and Q-Links.json for OP-ARCADE (strict AT20-proven schema)."""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIN_DIR = os.path.join(ROOT, "plugins", "op_arcade", "skins")
MANIFEST = os.path.join(ROOT, "tools", "ui_manifest.json")


class DupObj:
    def __init__(self, pairs):
        self.pairs = pairs


def dump_mpc_json(obj, indent=0):
    sp = "  " * indent
    sp2 = "  " * (indent + 1)
    if isinstance(obj, DupObj):
        items = [f'{sp2}{json.dumps(k)}: {dump_mpc_json(v, indent + 1)}' for k, v in obj.pairs]
        return "{\n" + ",\n".join(items) + f"\n{sp}}}"
    if isinstance(obj, dict):
        if not obj:
            return "{}"
        items = [f'{sp2}{json.dumps(k)}: {dump_mpc_json(v, indent + 1)}' for k, v in obj.items()]
        return "{\n" + ",\n".join(items) + f"\n{sp}}}"
    if isinstance(obj, list):
        if not obj:
            return "[]"
        if all(not isinstance(x, (dict, list, DupObj)) for x in obj):
            return "[" + ", ".join(json.dumps(x) for x in obj) + "]"
        items = [f"{sp2}{dump_mpc_json(x, indent + 1)}" for x in obj]
        return "[\n" + ",\n".join(items) + f"\n{sp}]"
    return json.dumps(obj)


def empty_bg():
    return {
        "version": 1,
        "focussed":   {"version": 1, "colour": "0", "image": ""},
        "unfocussed": {"version": 1, "colour": "0", "image": ""},
    }


def make_bounds(x, y, w, h, focus="No", invalid="Show", when="Always"):
    return {
        "version": 1,
        "acceptsHWFocus": focus,
        "showWhenDataModelInvalid": invalid,
        "whenVisible": when,
        "boundsType": "Absolute",
        "bounds": f"{int(round(x))} {int(round(y))} {int(round(w))} {int(round(h))}",
    }


def make_entry(name, comp_type, data, bounds, remap=None):
    return {
        "version": 2,
        "componentData": {"version": 1, "name": name, "type": comp_type, "data": data},
        "handle remapping": {"version": 1, "map": remap if remap is not None else []},
        "bounds": bounds,
    }


def make_knob_overlay_def(bipolar=False):
    overlay_data = DupObj([
        ("version", 2),
        ("version", 1),
        ("actions", [
            {"version": 1, "onAction": "Focus Lost", "handler": "Focus Lost", "handleName": "Focus Lost", "additionalData": ""}
        ]),
        ("overlayType", "Knob With Header"),
        ("scaleType", "Linear"),
        ("polarityType", "Bipolar" if bipolar else "Unipolar"),
        ("showMinusSign", True),
        ("handleName", "Data"),
    ])
    return {
        "version": 2,
        "actions": [],
        "backgroundData": empty_bg(),
        "ignoreMousePresses": True,
        "disableCoarseDataWheel": False,
        "componentsData": [
            make_entry("overlay", "KnobOverlay", overlay_data,
                       make_bounds(0, 0, 1280, 800, focus="Yes", invalid="Show"))
        ],
    }


def make_knob_component_def(strip, frames, w, h, bipolar=False):
    overlay_name = "knob overlay bipolar" if bipolar else "knob overlay"
    return {
        "version": 2,
        "actions": [
            {"version": 1, "onAction": "Mouse Down",    "handler": "Q-Link",       "handleName": "Data", "additionalData": ""},
            {"version": 1, "onAction": "Double Click",  "handler": "Show Overlay", "handleName": "Data", "additionalData": overlay_name},
            {"version": 1, "onAction": "Enter Pressed", "handler": "Show Overlay", "handleName": "Data", "additionalData": overlay_name},
        ],
        "backgroundData": empty_bg(),
        "ignoreMousePresses": False,
        "disableCoarseDataWheel": False,
        "componentsData": [
            make_entry(
                "focus", "Focus",
                {"version": 1, "backgroundColour": "14dca430", "outlineColour": "ffdca430",
                 "backgroundInset": 1.0, "outlineThickness": 1.5},
                make_bounds(0, 0, w, h, focus="No", invalid="Show", when="WhenFocussed"),
            ),
            make_entry(
                "knob", "Knob",
                {"version": 3, "knobType": "FilmStrip", "filmStrip": strip,
                 "numFrames": frames, "invert": False, "dragOrientation": "Vertical",
                 "handleName": "Data"},
                make_bounds(0, 0, w, h, focus="No", invalid="Show"),
            ),
        ],
    }


def make_qlink_dict(cols_top_to_bottom):
    slots = [
        [13, 9, 5, 1],
        [14, 10, 6, 2],
        [15, 11, 7, 3],
        [16, 12, 8, 4],
    ]
    out = {}
    for c in range(4):
        for r in range(4):
            out[f"Q-Link {slots[c][r]}"] = cols_top_to_bottom[c][r]
    return {f"Q-Link {i}": out[f"Q-Link {i}"] for i in range(1, 17)}


def main():
    with open(MANIFEST, "r", encoding="utf-8") as fh:
        m = json.load(fh)

    local_defs = [
        {"key": "knob overlay",         "value": make_knob_overlay_def(bipolar=False)},
        {"key": "knob overlay bipolar", "value": make_knob_overlay_def(bipolar=True)},
    ]
    seen = {}

    def knob_type(k):
        key = (k["strip"], k["frames"], k["w"], k["h"], bool(k.get("bipolar", False)))
        if key not in seen:
            tn = f"k_{os.path.splitext(k['strip'])[0]}_{k['w']}x{k['h']}_{'bi' if key[4] else 'uni'}"
            seen[key] = tn
            local_defs.append({
                "key": tn,
                "value": make_knob_component_def(k["strip"], k["frames"], k["w"], k["h"], bipolar=key[4]),
            })
        return seen[key]

    mul_names = ["0.5", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"]
    dt1_names = ["-3", "-2", "-1", "0", "+1", "+2", "+3"]
    dt2_names = ["1.00", "1.41", "1.57", "1.73"]
    repl_rows = [
        (1,  ["OPM 2151", "OPN2 2612", "HYBRID"]),
        (2,  [f"ALG {i}" for i in range(8)]),
        (3,  [str(i) for i in range(8)]),
        (4,  ["Poly", "Mono", "Unison 2"]),
        (14, ["Crisp", "MD1 Warm", "Lo-Fi"]),
        (54, ["-1", "0", "+1"]),
        (57, ["Off", "Loop", "Alt", "Inv"]),
        (59, ["Square", "Sub -1", "Sub -2", "Pulse", "Noise"]),
        (63, [str(i) for i in range(13)]),
        (64, ["Saw", "Square", "Tri", "Noise"]),
        (65, ["1/16", "1/8T", "1/8", "1/8D", "1/4", "1/2"]),
        (72, ["LP12", "LP24", "BP12", "HP12"]),
    ]
    for op in range(4):
        b = 17 + op * 8
        repl_rows.append((b + 0, mul_names))
        repl_rows.append((b + 1, dt1_names))
        repl_rows.append((b + 2, dt2_names))
    for op in range(4):
        repl_rows.append((49 + op, ["Off", "On"]))

    tabs = []
    for tname, fn_key, fn_sub, bg_file, spec in [
        ("OP3000", 0, 0, "bg_arcade.png", m["page1"]),
        ("OP 1-2", 1, 0, "bg_ops.png",    m["page2"]),
        ("OP 3-4", 1, 1, "bg_ops.png",    m["page2"]),
        ("HYBRID", 2, 0, "bg_pcb.png",    m["page3"]),
        ("MOD/AM", 2, 1, "bg_pcb.png",    m["page3"]),
        ("FILTER", 3, 0, "bg_filter.png", m["page4"]),
    ]:
        comps = [
            make_entry("bg", "Image",
                       {"version": 2, "imageType": "Regular", "colour": "0", "image": bg_file},
                       make_bounds(0, 0, 1280, 628, focus="No", invalid="Show"))
        ]
        for st in spec.get("strips", []):
            comps.append(make_entry(
                st["id"], "Image",
                {"version": 2, "imageType": "FilmStrip", "colour": "0",
                 "image": st["strip"], "numFrames": st["frames"],
                 "handleName": f"Parameter {st['param']}"},
                make_bounds(st["cx"] - st["w"]/2, st["cy"] - st["h"]/2, st["w"], st["h"], focus="No", invalid="Show"),
            ))
        for lb in spec.get("labels", []):
            comps.append(make_entry(
                lb["id"], "Label",
                {
                    "version": 1,
                    "textStyle": {
                        "version": 1,
                        "font": {"version": 1, "name": "Titillium Web", "style": "SemiBold", "height": float(lb["size"])},
                        "colour": lb["color"],
                        "justification": "horizontallyCentred verticallyCentred",
                        "case": "Original",
                    },
                    "type": "Value",
                    "handleName": f"Parameter {lb['param']}",
                },
                make_bounds(lb["x"], lb["y"], lb["w"], lb["h"], focus="No", invalid="Show"),
            ))
        for k in spec.get("knobs", []):
            comps.append(make_entry(
                k["id"], knob_type(k),
                {"version": 1, "handleName": "Data"},
                make_bounds(k["cx"] - k["w"]/2, k["cy"] - k["h"]/2, k["w"], k["h"], focus="Yes", invalid="Hide"),
                remap=[{"key": "Data", "value": f"Parameter {k['param']}"}],
            ))

        tabs.append({
            "version": 1,
            "tabName": tname,
            "fnKeyIndex": fn_key,
            "fnKeySubIndex": fn_sub,
            "componentDefinition": {
                "version": 2,
                "actions": [],
                "backgroundData": {"version": 1, "focussed": {"version": 1, "colour": "ff1f1e20", "image": ""}, "unfocussed": {"version": 1, "colour": "ff1f1e20", "image": ""}},
                "ignoreMousePresses": False,
                "disableCoarseDataWheel": False,
                "componentsData": comps,
            },
            "qlinkBoundsData": [],
        })

    tui = {
        "pageData": {
            "version": 1,
            "componentDefinitions": {
                "version": 2,
                "importFiles": [],
                "localComponentDefinitions": local_defs,
            },
            "replacementStrings": [{"key": f"Parameter {p}", "value": v} for p, v in repl_rows],
            "info": {"version": 1, "type": "CompleteDescription"},
            "tabs": tabs,
        }
    }

    qlinks = {
        "version": 4,
        "info": {"version": 1, "type": "CompleteDescription"},
        "Screen Mode Q-Links": {
            "version": 4,
            "map": [
                {
                    "Tab": 1, "SubTab": 1, "Bank Direction": "Column",
                    "Q-Links": make_qlink_dict([
                        [1, 2, 3, 4],     # Chip, Alg, Feedback, Voice
                        [5, 6, 7, 8],     # FM Bite, Attack, Decay, Release
                        [9, 10, 11, 12],  # Detune, LFO Speed, Vib, Trem
                        [13, 14, 15, 16], # PSG Sub, PCB Tone, Chorus, Delay
                    ]),
                },
                {
                    "Tab": 2, "SubTab": 1, "Bank Direction": "Column",
                    "Q-Links": make_qlink_dict([
                        [17, 20, 18, 19], # OP1 Ratio, Level, Detune, Coarse
                        [21, 22, 23, 24], # OP1 Attack, Decay, Sustain, Release
                        [25, 28, 26, 27], # OP2 Ratio, Level, Detune, Coarse
                        [29, 30, 31, 32], # OP2 Attack, Decay, Sustain, Release
                    ]),
                },
                {
                    "Tab": 2, "SubTab": 2, "Bank Direction": "Column",
                    "Q-Links": make_qlink_dict([
                        [33, 36, 34, 35], # OP3 Ratio, Level, Detune, Coarse
                        [37, 38, 39, 40], # OP3 Attack, Decay, Sustain, Release
                        [41, 44, 42, 43], # OP4 Ratio, Level, Detune, Coarse
                        [45, 46, 47, 48], # OP4 Attack, Decay, Sustain, Release
                    ]),
                },
                {
                    "Tab": 3, "SubTab": 1, "Bank Direction": "Column",
                    "Q-Links": make_qlink_dict([
                        [53, 54, 55, 58], # Blend, OPN2 Oct, Det, DAC Grit
                        [56, 57, 59, 60], # OPM Noise, SSG-EG, PSG Wave, Decay
                        [61, 62, 63, 64], # Vel, Glide, Bend, LFO Wave
                        [65, 66, 67, 0],  # Dly Time, Fbk, Tone, Volume
                    ]),
                },
                {
                    "Tab": 3, "SubTab": 2, "Bank Direction": "Column",
                    "Q-Links": make_qlink_dict([
                        [49, 50, 51, 52], # OP1..4 AM Enable
                        [10, 64, 11, 12], # LFO Speed, Wave, Vib, Trem
                        [2, 3, 5, 9],     # Algorithm, Feedback, FM Bite, Detune
                        [1, 53, 13, 0],   # Chip Mode, Blend, PSG Sub, Volume
                    ]),
                },
                {
                    "Tab": 4, "SubTab": 1, "Bank Direction": "Column",
                    "Q-Links": make_qlink_dict([
                        [73, 74, 72, 75], # Cutoff, Resonance, Filter Mode, HPF Cut
                        [78, 79, 77, 76], # Env Depth, Filt Vel, Keytrack, OTA Drive
                        [80, 81, 82, 83], # Filt Attack, Decay, Sustain, Release
                        [84, 10, 5,  0],  # LFO -> Cut, LFO Speed, FM Bite, Volume
                    ]),
                },
            ],
        },
        "Program Mode Q-Links": {
            "Q-Link 1": 5,   # FM Bite
            "Q-Link 2": 2,   # Algorithm
            "Q-Link 3": 3,   # Feedback
            "Q-Link 4": 1,   # Chip Mode
            "Q-Link 5": 6,   # Attack
            "Q-Link 6": 7,   # Decay
            "Q-Link 7": 8,   # Release
            "Q-Link 8": 9,   # Detune
            "Q-Link 9": 13,  # PSG Sub
            "Q-Link 10": 14, # PCB Tone
            "Q-Link 11": 15, # Chorus
            "Q-Link 12": 16, # Delay Mix
            "Q-Link 13": 10, # LFO Speed
            "Q-Link 14": 11, # Vib / PM
            "Q-Link 15": 53, # Chip Blend
            "Q-Link 16": 0,  # Volume
        },
    }

    with open(os.path.join(SKIN_DIR, "TUI.json"), "w", encoding="utf-8") as fh:
        fh.write(dump_mpc_json(tui) + "\n")
    with open(os.path.join(SKIN_DIR, "Q-Links.json"), "w", encoding="utf-8") as fh:
        fh.write(dump_mpc_json(qlinks) + "\n")
    print(f"Wrote TUI.json & Q-Links.json to {SKIN_DIR}")


if __name__ == "__main__":
    main()
