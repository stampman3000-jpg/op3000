#pragma once
// OP-ARCADE — 32 Handcrafted Classic Arcade & Mega Drive 4-Op FM Presets.
// Every preset drives real YM2151 (OPM) & YM2612 (OPN2) hardware FM operators + PSG + FX.

#include "../dsp/OpArcadeEngine.h"

namespace oparcade {

struct ArcadePreset
{
    const char* name;
    SynthParams p;
};

inline SynthParams makePreset (
    int chipMode, int alg, int fb, int voiceMode,
    float fmBite, float detune, float lfoSpd, float lfoPm, float lfoAm,
    float psgMix, int pcbTone, float chMix, float dlyMix,
    OpParams o1, OpParams o2, OpParams o3, OpParams o4,
    float blend = 0.5f, int opn2Oct = 0, float opn2Det = 0.18f,
    int opmNoise = 0, int ssgEg = 0, float dacGrit = 0.0f,
    int psgWave = 1, float psgDec = 0.35f, float velSens = 0.55f,
    float porta = 0.0f, int dlyDiv = 3, float dlyFb = 0.35f)
{
    SynthParams s;
    s.volume = 0.80f;
    s.chipMode = chipMode; s.alg = alg; s.feedback = fb; s.voiceMode = voiceMode;
    s.fmBite = fmBite; s.detune = detune; s.lfoSpeed = lfoSpd; s.lfoPm = lfoPm; s.lfoAm = lfoAm;
    s.psgMix = psgMix; s.pcbTone = pcbTone; s.chorusMix = chMix; s.delayMix = dlyMix;
    s.op[0] = o1; s.op[1] = o2; s.op[2] = o3; s.op[3] = o4;
    s.chipBlend = blend; s.opn2Oct = opn2Oct; s.opn2Detune = opn2Det;
    s.opmNoise = opmNoise; s.ssgEg = ssgEg; s.dacGrit = dacGrit;
    s.psgWave = psgWave; s.psgDecay = psgDec; s.velSens = velSens;
    s.porta = porta; s.delayDiv = dlyDiv; s.delayFb = dlyFb;
    return s;
}

inline const ArcadePreset* factoryPresets (int& outCount)
{
    // OpParams: { mul, dt1, dt2, level%, ar, d1r, d2r, sl, rr, am }
    static const ArcadePreset kPresets[] = {
        // 0: Huge dual-chip stereo FM brass.
        { "Outrun-ish Brass", makePreset (
            2, 4, 6, 0,  0.0f, 0.30f, 0.40f, 0.12f, 0.0f,  0.0f, 0, 0.38f, 0.22f,
            { 1,  1, 0, 74, 24,  9, 0,  3,  6, 0 },
            { 1, -1, 0, 94, 26,  4, 0,  2,  7, 0 },
            { 1,  2, 0, 68, 22, 10, 0,  4,  6, 0 },
            { 2, -2, 0, 90, 25,  5, 0,  2,  7, 0 },
            0.50f, 0, 0.22f) },

        // 1: Iconic TX81Z / Mega Drive "Lately / Solid" FM Bass!
        { "Lately FM Bass (OPN2)", makePreset (
            1, 2, 6, 1,  0.0f, 0.10f, 0.30f, 0.0f,  0.0f,  0.0f, 1, 0.08f, 0.0f,
            { 1,  0, 0, 72, 31, 14, 2,  5,  9, 0 },
            { 2,  1, 0, 66, 31, 16, 4,  7,  9, 0 },
            { 1, -1, 0, 78, 31, 11, 2,  4,  8, 0 },
            { 1,  0, 0, 98, 31,  7, 1,  3,  8, 0 },
            0.5f, 0, 0.0f, 0, 0, 0.0f, 1, 0.25f, 0.65f, 0.0f) },

        // 2: Streets of Rage punchy slap FM bass
        { "Streets Slap (OPN2)", makePreset (
            1, 4, 7, 1,  0.10f, 0.08f, 0.35f, 0.0f, 0.0f,  0.0f, 0, 0.10f, 0.0f,
            { 3,  0, 0, 78, 31, 17, 6,  9, 10, 0 },
            { 1,  0, 0, 96, 31,  9, 2,  4,  9, 0 },
            { 7,  1, 0, 56, 31, 22, 8, 12, 12, 0 },
            { 1, -1, 0, 92, 31, 10, 2,  4,  9, 0 }) },

        // 3: Crisp Sega System 16 / Capcom CPS-1 Synth Lead
        { "CPS-1 Arcade Lead", makePreset (
            0, 0, 6, 1,  0.05f, 0.20f, 0.52f, 0.22f, 0.0f, 0.0f, 0, 0.30f, 0.28f,
            { 2,  2, 0, 68, 31,  9, 0,  3,  8, 0 },
            { 1, -1, 0, 74, 31,  7, 0,  2,  8, 0 },
            { 1,  1, 0, 72, 31,  6, 0,  2,  7, 0 },
            { 1,  0, 0, 96, 31,  4, 0,  1,  8, 0 },
            0.5f, 0, 0.0f, 0, 0, 0.0f, 1, 0.3f, 0.5f, 0.18f) },

        // 4: Glassy Sega Crystal Rhodes / Electric Piano
        { "Crystal Rhodes (OPM)", makePreset (
            0, 4, 3, 0,  0.0f, 0.18f, 0.32f, 0.05f, 0.18f, 0.0f, 0, 0.42f, 0.24f,
            { 14, 0, 0, 52, 31, 17, 6, 10,  9, 0 },
            { 1,  1, 0, 92, 31,  7, 3,  4,  7, 1 },
            { 1, -1, 0, 62, 31, 12, 3,  6,  8, 0 },
            { 1,  0, 0, 95, 31,  6, 2,  3,  7, 0 }) },

        // 5: Iconic Sonic Ring / X68000 DT2 Chime Bell!
        { "Sonic DT2 Bell (OPM)", makePreset (
            0, 4, 4, 0,  0.0f, 0.15f, 0.30f, 0.0f,  0.0f,  0.0f, 0, 0.35f, 0.34f,
            { 7,  1, 2, 74, 31, 11, 4,  7,  5, 0 },
            { 2, -1, 0, 92, 31,  8, 3,  6,  5, 0 },
            { 3,  2, 3, 66, 31, 13, 5,  8,  5, 0 },
            { 1,  0, 0, 94, 31,  7, 3,  5,  5, 0 }) },

        // 6: Massive Dual-Chip Hybrid Stack (-1 oct OPN2 sub + OPM sparkle)
        { "Cyber Hybrid Stack", makePreset (
            2, 5, 5, 0,  0.0f, 0.35f, 0.36f, 0.12f, 0.0f,  0.22f, 0, 0.45f, 0.26f,
            { 1,  0, 0, 72, 29,  8, 0,  3,  6, 0 },
            { 1, -2, 0, 92, 30,  5, 0,  2,  6, 0 },
            { 2,  2, 0, 86, 28,  6, 0,  3,  6, 0 },
            { 4, -1, 0, 78, 30,  9, 1,  4,  6, 0 },
            0.48f, -1, 0.28f) },

        // 7: Lush Sharp X68000 FM Ensemble Strings
        { "X68000 Strings (OPM)", makePreset (
            0, 4, 5, 0, -0.08f, 0.40f, 0.34f, 0.18f, 0.0f, 0.0f, 0, 0.65f, 0.25f,
            { 1,  2, 0, 66, 15,  4, 0,  2,  4, 0 },
            { 1, -2, 0, 90, 14,  2, 0,  1,  4, 0 },
            { 2,  1, 0, 58, 13,  5, 0,  2,  4, 0 },
            { 1,  3, 0, 88, 15,  2, 0,  1,  4, 0 }) },

        // 8: Warm Mega Drive Model-1 Choir Pad
        { "MD1 Choir Pad (OPN2)", makePreset (
            1, 6, 4, 0, -0.10f, 0.38f, 0.28f, 0.16f, 0.22f, 0.0f, 1, 0.68f, 0.30f,
            { 2,  1, 0, 56, 16,  5, 0,  2,  4, 0 },
            { 1, -2, 0, 88, 15,  2, 0,  1,  4, 1 },
            { 2,  2, 0, 74, 14,  3, 0,  1,  4, 0 },
            { 3, -1, 0, 68, 13,  4, 0,  2,  4, 1 }) },

        // 9: Classic 80s Arcade Jazz / Rock Organ (Alg 7 Additive!)
        { "Arcade Organ (OPM)", makePreset (
            0, 7, 4, 0,  0.0f, 0.12f, 0.62f, 0.15f, 0.20f, 0.0f, 0, 0.48f, 0.12f,
            { 1,  0, 0, 92, 31,  0, 0,  0,  9, 0 },
            { 2,  1, 0, 86, 31,  0, 0,  0,  9, 1 },
            { 3, -1, 0, 78, 31,  0, 0,  0,  9, 0 },
            { 4,  0, 0, 72, 31,  6, 0,  4,  9, 1 }) },

        // 10: Punchy Capcom Fighter Synth Brass
        { "Capcom Brass (OPM)", makePreset (
            0, 3, 6, 0,  0.10f, 0.24f, 0.42f, 0.10f, 0.0f, 0.0f, 0, 0.30f, 0.20f,
            { 1,  1, 0, 72, 23,  8, 0,  3,  7, 0 },
            { 1, -2, 0, 78, 25,  6, 0,  2,  7, 0 },
            { 2,  2, 0, 64, 21, 10, 0,  4,  7, 0 },
            { 1,  0, 0, 96, 26,  4, 0,  1,  7, 0 }) },

        // 11: Gritty MD1 Sub Bass + PSG Square Stack
        { "Mega Sub + PSG (HYB)", makePreset (
            2, 1, 5, 1,  0.0f, 0.12f, 0.25f, 0.0f,  0.0f,  0.48f, 1, 0.12f, 0.0f,
            { 1,  0, 0, 68, 31, 13, 2,  5,  9, 0 },
            { 0,  1, 0, 64, 31, 15, 3,  6,  9, 0 },
            { 1, -1, 0, 74, 31, 10, 1,  4,  8, 0 },
            { 0,  0, 0, 98, 31,  6, 0,  2,  8, 0 },
            0.65f, -1, 0.12f, 0, 0, 0.0f, 1, 0.35f) },

        // 12: Authentic Chiptune Arp Square + FM Pluck
        { "Chiptune Stack (HYB)", makePreset (
            2, 4, 5, 0,  0.0f, 0.16f, 0.45f, 0.08f, 0.0f,  0.70f, 0, 0.20f, 0.25f,
            { 2,  0, 0, 68, 31, 16, 4,  8,  9, 0 },
            { 1,  1, 0, 88, 31, 12, 3,  6,  8, 0 },
            { 4, -1, 0, 58, 31, 18, 5,  9,  9, 0 },
            { 2,  0, 0, 84, 31, 13, 3,  6,  8, 0 },
            0.50f, 0, 0.15f, 0, 0, 0.0f, 3, 0.22f) },

        // 13: Funky Genesis FM Clavinet
        { "Genesis Clav (OPN2)", makePreset (
            1, 4, 7, 0,  0.12f, 0.08f, 0.30f, 0.0f,  0.0f,  0.0f, 0, 0.15f, 0.10f,
            { 3,  0, 0, 78, 31, 15, 5,  8, 10, 0 },
            { 1,  1, 0, 94, 31, 11, 4,  6,  9, 0 },
            { 5, -1, 0, 64, 31, 18, 6, 10, 10, 0 },
            { 1,  0, 0, 92, 31, 12, 4,  6,  9, 0 }) },

        // 14: Wooden Arcade Marimba / Mallet
        { "Arcade Marimba (OPM)", makePreset (
            0, 4, 2, 0,  0.0f, 0.08f, 0.30f, 0.0f,  0.0f,  0.0f, 0, 0.20f, 0.26f,
            { 4,  0, 0, 68, 31, 20, 8, 11, 10, 0 },
            { 1,  0, 0, 95, 31, 13, 5,  9,  8, 0 },
            { 10, 1, 0, 46, 31, 25, 10, 13, 11, 0 },
            { 1, -1, 0, 90, 31, 14, 5,  9,  8, 0 }) },

        // 15: Shimmering Tokyo Drift FM Harp
        { "Tokyo FM Harp (OPM)", makePreset (
            0, 4, 4, 0,  0.0f, 0.22f, 0.35f, 0.06f, 0.0f,  0.0f, 0, 0.45f, 0.38f,
            { 3,  1, 0, 66, 31, 13, 4,  8,  6, 0 },
            { 1, -1, 0, 94, 31,  9, 3,  6,  5, 0 },
            { 6,  2, 1, 52, 31, 16, 5, 10,  6, 0 },
            { 2,  0, 0, 86, 31, 10, 3,  6,  5, 0 }) },

        // 16: Industrial DT2 Metallic Steel Perc
        { "Steel DT2 Perc (OPM)", makePreset (
            0, 0, 7, 0,  0.22f, 0.20f, 0.40f, 0.0f,  0.0f,  0.0f, 0, 0.18f, 0.28f,
            { 5,  2, 3, 84, 31, 14, 5,  9,  7, 0 },
            { 3, -2, 2, 82, 31, 15, 5,  9,  7, 0 },
            { 7,  1, 1, 76, 31, 16, 6, 10,  7, 0 },
            { 1,  0, 0, 96, 31, 12, 4,  8,  6, 0 }) },

        // 17: Glass Harmonica / Ambient Crystal
        { "Glass Crystal (OPM)", makePreset (
            0, 5, 2, 0, -0.05f, 0.26f, 0.28f, 0.12f, 0.25f, 0.0f, 0, 0.55f, 0.40f,
            { 7,  0, 1, 56, 20,  9, 2,  5,  4, 0 },
            { 1,  1, 0, 88, 19,  5, 1,  3,  4, 1 },
            { 2, -1, 0, 84, 18,  6, 2,  4,  4, 0 },
            { 4,  2, 0, 76, 21,  8, 2,  5,  4, 1 }) },

        // 18: Dark Boss Unison FM Bass (Hybrid)
        { "Dark Boss Bass (HYB)", makePreset (
            2, 0, 7, 2,  0.18f, 0.42f, 0.45f, 0.08f, 0.0f,  0.25f, 0, 0.20f, 0.12f,
            { 1,  2, 0, 80, 31, 11, 2,  4,  8, 0 },
            { 2, -2, 0, 76, 31, 12, 2,  5,  8, 0 },
            { 1,  1, 0, 82, 31,  9, 1,  3,  8, 0 },
            { 0,  0, 0, 98, 31,  6, 0,  2,  8, 0 },
            0.55f, 0, 0.35f) },

        // 19: Squelchy FM Acid Resonance (OPN2)
        { "FM Acid Squelch", makePreset (
            1, 0, 7, 1,  0.28f, 0.05f, 0.50f, 0.0f,  0.0f,  0.0f, 0, 0.15f, 0.26f,
            { 1,  0, 0, 82, 31, 14, 4,  7,  9, 0 },
            { 4,  0, 0, 78, 31, 16, 5,  8,  9, 0 },
            { 2,  1, 0, 80, 31, 12, 3,  6,  9, 0 },
            { 1,  0, 0, 96, 31,  8, 2,  4,  9, 0 },
            0.5f, 0, 0.0f, 0, 0, 0.0f, 1, 0.25f, 0.70f, 0.24f) },

        // 20: Sega Mega Drive SSG-EG Looping Sci-Fi Synth!
        { "SSG-EG Sci-Fi (OPN2)", makePreset (
            1, 4, 6, 0,  0.10f, 0.25f, 0.65f, 0.25f, 0.0f,  0.0f, 0, 0.35f, 0.36f,
            { 3,  1, 0, 78, 31, 14, 0,  8,  6, 0 },
            { 1, -1, 0, 94, 31, 14, 0,  9,  6, 0 },
            { 5,  2, 0, 72, 31, 16, 0, 10,  6, 0 },
            { 2,  0, 0, 90, 31, 15, 0,  9,  6, 0 },
            0.5f, 0, 0.0f, 0, 2, 0.0f) }, // ssgEg = 2 (Alternating loop!)

        // 21: Deep Arcade Tubular Bells
        { "Tubular Bells (OPM)", makePreset (
            0, 4, 5, 0,  0.05f, 0.15f, 0.25f, 0.0f,  0.0f,  0.0f, 0, 0.32f, 0.35f,
            { 7,  0, 3, 76, 31, 10, 3,  7,  4, 0 },
            { 2,  1, 0, 92, 31,  7, 2,  5,  4, 0 },
            { 11, 2, 2, 62, 31, 13, 4,  9,  4, 0 },
            { 1, -1, 0, 96, 31,  6, 2,  4,  4, 0 }) },

        // 22: Warm Mega Drive MD1 Analog Poly Synth
        { "Warm MD1 Poly (OPN2)", makePreset (
            1, 4, 5, 0, -0.05f, 0.32f, 0.36f, 0.14f, 0.0f,  0.0f, 1, 0.52f, 0.24f,
            { 1,  2, 0, 68, 25,  8, 1,  4,  6, 0 },
            { 1, -2, 0, 92, 27,  5, 0,  2,  6, 0 },
            { 2,  1, 0, 60, 23, 10, 1,  5,  6, 0 },
            { 1,  3, 0, 90, 26,  5, 0,  2,  6, 0 }) },

        // 23: Lo-Fi 16-Bit Grit Electric Keys
        { "Lo-Fi 16-Bit Keys", makePreset (
            1, 4, 4, 0,  0.0f, 0.24f, 0.30f, 0.10f, 0.15f,  0.0f, 2, 0.38f, 0.28f,
            { 6,  1, 0, 60, 31, 15, 5,  9,  7, 0 },
            { 1, -1, 0, 94, 31,  8, 3,  5,  6, 1 },
            { 2,  2, 0, 64, 31, 13, 4,  7,  7, 0 },
            { 1,  0, 0, 92, 31,  7, 3,  4,  6, 0 },
            0.5f, 0, 0.0f, 0, 0, 0.45f) },

        // 24: Space Harrier Punchy Hybrid Pluck
        { "Harrier Pluck (HYB)", makePreset (
            2, 4, 6, 0,  0.08f, 0.28f, 0.40f, 0.06f, 0.0f,  0.18f, 0, 0.35f, 0.32f,
            { 2,  1, 0, 76, 31, 14, 4,  8,  8, 0 },
            { 1, -1, 0, 94, 31, 10, 3,  6,  7, 0 },
            { 3,  2, 0, 68, 31, 16, 5,  9,  8, 0 },
            { 1,  0, 0, 92, 31, 11, 3,  6,  7, 0 },
            0.45f, 0, 0.20f) },

        // 25: Punchy FM Kick Drum (play low C2-C3!)
        { "FM Kick Drum (OPN2)", makePreset (
            1, 0, 7, 1,  0.15f, 0.0f, 0.20f, 0.0f,  0.0f,   0.0f, 1, 0.0f,  0.0f,
            { 2,  0, 0, 82, 31, 24, 12, 13, 12, 0 },
            { 1,  0, 0, 78, 31, 21, 10, 12, 11, 0 },
            { 0,  0, 0, 74, 31, 19,  8, 11, 10, 0 },
            { 0,  0, 0, 98, 31, 14,  6, 10,  9, 0 }) },

        // 26: YM2151 Hardware Noise Arcade Snare / Hi-Hat!
        { "Arcade Noise Perc", makePreset (
            0, 6, 7, 0,  0.20f, 0.10f, 0.30f, 0.0f,  0.0f,  0.35f, 0, 0.08f, 0.14f,
            { 3,  0, 2, 82, 31, 20, 8, 12, 11, 0 },
            { 1,  1, 0, 90, 31, 17, 7, 11, 10, 0 },
            { 5, -2, 3, 76, 31, 21, 9, 13, 11, 0 },
            { 2,  0, 0, 96, 31, 18, 8, 12, 10, 0 },
            0.5f, 0, 0.0f, 22, 0, 0.0f, 4, 0.16f) }, // opmNoise=22 + PSG LFSR noise!

        // 27: Electro FM Cowbell / Synth Tom
        { "Electro FM Cowbell", makePreset (
            0, 4, 5, 0,  0.10f, 0.08f, 0.30f, 0.0f,  0.0f,  0.0f, 0, 0.12f, 0.20f,
            { 3,  2, 2, 74, 31, 17, 6, 10,  9, 0 },
            { 2, -1, 0, 94, 31, 14, 5,  9,  8, 0 },
            { 5,  1, 3, 66, 31, 19, 7, 11,  9, 0 },
            { 1,  0, 0, 90, 31, 15, 5,  9,  8, 0 }) },

        // 28: Dreamy Vaporwave Hybrid FM E-Piano
        { "Vaporwave EP (HYB)", makePreset (
            2, 4, 3, 0, -0.08f, 0.34f, 0.26f, 0.14f, 0.20f, 0.0f, 1, 0.62f, 0.38f,
            { 12, 1, 0, 50, 31, 16, 5, 10,  7, 0 },
            { 1, -1, 0, 92, 31,  7, 2,  4,  6, 1 },
            { 1,  2, 0, 58, 31, 12, 3,  6,  7, 0 },
            { 2, -2, 0, 86, 31,  8, 3,  5,  6, 0 },
            0.52f, 0, 0.32f) },

        // 29: Mega Drive Hero Overdrive FM Guitar
        { "MD Hero Guitar (OPN2)", makePreset (
            1, 0, 7, 1,  0.22f, 0.16f, 0.56f, 0.24f, 0.0f,  0.0f, 0, 0.24f, 0.28f,
            { 2,  1, 0, 84, 31, 11, 2,  4,  8, 0 },
            { 1, -2, 0, 82, 31,  8, 1,  3,  8, 0 },
            { 3,  2, 0, 72, 31, 12, 2,  5,  8, 0 },
            { 1,  0, 0, 96, 31,  6, 1,  2,  8, 0 },
            0.5f, 0, 0.0f, 0, 0, 0.0f, 1, 0.3f, 0.5f, 0.14f) },

        // 30: Shimmering Ambient Dual-Chip Pad
        { "Shimmer Pad (HYB)", makePreset (
            2, 5, 4, 0, -0.06f, 0.44f, 0.22f, 0.15f, 0.28f, 0.0f, 0, 0.72f, 0.45f,
            { 1,  0, 0, 62, 12,  4, 0,  2,  3, 0 },
            { 1, -2, 0, 88, 11,  2, 0,  1,  3, 1 },
            { 2,  2, 1, 80, 10,  3, 0,  2,  3, 0 },
            { 4, -1, 2, 72,  9,  4, 1,  2,  3, 1 },
            0.46f, 1, 0.34f, 0, 0, 0.0f, 1, 0.4f, 0.4f, 0.0f, 4, 0.48f) },

        // 31: Clean Init Pure Sine (for starting patches from scratch)
        { "Init Pure Sine", makePreset (
            0, 0, 0, 0,  0.0f, 0.0f, 0.35f, 0.0f,  0.0f,    0.0f, 0, 0.0f,  0.0f,
            { 1,  0, 0,  0, 31,  0, 0,  0,  7, 0 },
            { 1,  0, 0,  0, 31,  0, 0,  0,  7, 0 },
            { 1,  0, 0,  0, 31,  0, 0,  0,  7, 0 },
            { 1,  0, 0, 96, 31,  0, 0,  0,  7, 0 }) },
    };

    outCount = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
    return kPresets;
}

} // namespace oparcade
