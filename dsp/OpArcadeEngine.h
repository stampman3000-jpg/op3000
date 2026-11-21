#pragma once
// OP-ARCADE — Dual-Chip Yamaha YM2151 (OPM) + YM2612 (OPN2) Hybrid FM Synthesizer Engine.
// Uses authentic Yamaha 256-entry log-sine & exponential ROM tables (YamahaSiliconFm.h),
// exact 10-bit Yamaha EG attenuation, feedback bit-shifts, OPM DT2 inharmonic ratios & LFSR noise,
// YM2612 9-bit ladder DAC crossover distortion & SSG-EG looping envelopes, plus SN76489 PSG sub-chip,
// Mega Drive Model 1 filter, 80s Stereo Chorus, and Tempo-Synced Stereo Delay.
//
// Zero heap allocations, locks, or I/O in process(). Pure C++17.

#include "YamahaSiliconFm.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace oparcade {

constexpr float kPi        = 3.14159265358979323846f;
constexpr float kTwoPi     = 6.28318530717958647692f;
constexpr int   kMaxVoices = 8;

inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline int   clampi (int x, int lo, int hi)       { return x < lo ? lo : (x > hi ? hi : x); }

inline float fastTanh (float x)
{
    if (x > 3.5f) return 1.0f;
    if (x < -3.5f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline bool isCarrierOp (int alg, int op)
{
    alg = clampi (alg, 0, 7);
    if (op == 3) return true;
    if (op == 2) return alg >= 5;
    if (op == 1) return alg >= 4;
    return alg == 7;
}

struct OpParams
{
    int mul = 1;    // 0..15 (0 = 0.5x, 1..15 = 1x..15x)
    int dt1 = 0;    // -3..+3 fine detune
    int dt2 = 0;    // 0..3 OPM coarse harmonic detune (1.00, 1.41, 1.58, 1.73)
    int level = 85; // 0..100%
    int ar = 31;    // 0..31
    int d1r = 8;    // 0..31
    int d2r = 0;    // 0..31
    int sl = 4;     // 0..15
    int rr = 7;     // 0..15
    int am = 0;     // 0..1
};

struct SynthParams
{
    float volume     = 0.80f;
    int   chipMode   = 2;     // 0=OPM (YM2151), 1=OPN2 (YM2612), 2=HYBRID (Dual)
    int   alg        = 4;     // 0..7
    int   feedback   = 5;     // 0..7
    int   voiceMode  = 0;     // 0=Poly, 1=Mono, 2=Unison 2
    float fmBite     = 0.0f;  // -1..+1
    float macroAtt   = 0.0f;  // -1..+1
    float macroDec   = 0.0f;  // -1..+1
    float macroRel   = 0.0f;  // -1..+1
    float detune     = 0.20f; // 0..1
    float lfoSpeed   = 0.35f; // 0..1
    float lfoPm      = 0.15f; // 0..1
    float lfoAm      = 0.0f;  // 0..1
    float psgMix     = 0.0f;  // 0..1
    int   pcbTone    = 0;     // 0=Arcade Crisp, 1=Mega MD1 Warm LP, 2=Lo-Fi Grit
    float chorusMix  = 0.25f; // 0..1
    float delayMix   = 0.20f; // 0..1

    OpParams op[4];

    float chipBlend  = 0.50f;
    int   opn2Oct    = 0;     // -1, 0, +1
    float opn2Detune = 0.20f; // -1..+1 (-50..+50 ct)
    int   opmNoise   = 0;     // 0..31
    int   ssgEg      = 0;     // 0..3
    float dacGrit    = 0.0f;  // 0..1
    int   psgWave    = 1;     // 0..4
    float psgDecay   = 0.35f; // 0..1
    float velSens    = 0.55f; // 0..1
    float porta      = 0.0f;  // 0..1
    int   bendRange  = 2;     // 0..12
    int   lfoWave    = 2;     // 0=Saw, 1=Sqr, 2=Tri, 3=Noise
    int   delayDiv   = 3;     // 0..5
    float delayFb    = 0.35f; // 0..0.9
    float delayTone  = 0.65f; // 0..1

    // Page 4: Per-Voice Hybrid Analog Filter + ADSR Envelope (bypassed by default)
    int   filtMode   = 1;     // 0=LP12, 1=LP24, 2=BP12, 3=HP12
    float filtCut    = 1.0f;  // 0..1 (25 Hz .. 19 kHz exp)
    float filtRes    = 0.0f;  // 0..1
    float filtHpf    = 0.0f;  // 0..1 (0=off, 20 Hz .. 4.5 kHz exp)
    float filtDrive  = 0.0f;  // 0..1 OTA pre-drive
    float filtKey    = 0.50f; // 0..1 keytracking around C4
    float filtEnvAmt = 0.0f;  // -1..+1 (+/- 5 octaves)
    float filtVel    = 0.40f; // 0..1 velocity scaling on env depth
    float filtAtt    = 0.0f;  // 0..1 (~1 ms .. 3.5 s)
    float filtDec    = 0.38f; // 0..1 (~8 ms .. 6.0 s)
    float filtSus    = 0.35f; // 0..1 sustain level
    float filtRel    = 0.35f; // 0..1 (~8 ms .. 6.0 s)
    float filtLfo    = 0.0f;  // 0..1 (+/- 2.5 octaves)
};

// Lightweight 80s Hybrid Per-Voice Zero-Delay-Feedback TPT SVF Filter + Dedicated Pre-HPF + ADSR
struct HybridVoiceFilter
{
    enum EnvStage { EnvIdle = 0, EnvAtk, EnvDec, EnvRel };

    float    hp1 = 0.0f, hp2 = 0.0f;
    float    ic1 = 0.0f, ic2 = 0.0f, ic3 = 0.0f, ic4 = 0.0f;
    EnvStage stage = EnvIdle;
    float    env = 0.0f;

    void reset()
    {
        hp1 = hp2 = ic1 = ic2 = ic3 = ic4 = 0.0f;
        stage = EnvIdle;
        env = 0.0f;
    }

    void noteOn()
    {
        if (env < 1e-4f)
            hp1 = hp2 = ic1 = ic2 = ic3 = ic4 = 0.0f;
        stage = EnvAtk;
    }

    void noteOff()
    {
        if (stage != EnvIdle)
            stage = EnvRel;
    }

    bool isQuiet() const
    {
        return std::fabs (ic1) < 1e-4f && std::fabs (ic2) < 1e-4f
            && std::fabs (ic3) < 1e-4f && std::fabs (ic4) < 1e-4f
            && std::fabs (hp1) < 1e-4f && std::fabs (hp2) < 1e-4f;
    }

    inline void stepEnvSubBlock (int subN, float sr, float att, float dec, float sus, float rel)
    {
        const float fSub = (float) subN;
        if (stage == EnvAtk)
        {
            const float tAtt = 0.001f * std::pow (3500.0f, clampf (att, 0.0f, 1.0f));
            const float c = 1.0f - std::exp (-fSub / (tAtt * sr));
            env += (1.05f - env) * c;
            if (env >= 1.0f) { env = 1.0f; stage = EnvDec; }
        }
        else if (stage == EnvDec)
        {
            const float tDec = 0.008f * std::pow (750.0f, clampf (dec, 0.0f, 1.0f));
            const float c = 1.0f - std::exp (-fSub / (tDec * sr));
            env += (clampf (sus, 0.0f, 1.0f) - env) * c;
        }
        else if (stage == EnvRel)
        {
            const float tRel = 0.008f * std::pow (750.0f, clampf (rel, 0.0f, 1.0f));
            const float c = 1.0f - std::exp (-fSub / (tRel * sr));
            env *= (1.0f - c);
            if (env < 1e-4f) { env = 0.0f; stage = EnvIdle; }
        }
    }

    inline void processMonoBlock (float* buf, int n, float sr, float invSr,
                                  int midiNote, float vel, float lfoBi, const SynthParams& p)
    {
        const bool  doHpf   = p.filtHpf > 0.005f;
        const bool  doDrive = p.filtDrive > 0.005f;
        const float fcH     = doHpf ? clampf (20.0f * std::exp2f (p.filtHpf * 7.8138f), 20.0f, sr * 0.40f) : 20.0f;
        const float gH      = doHpf ? std::tan (kPi * fcH * invSr) : 0.0f;
        const float kH      = 1.41421356f;
        const float ah1     = 1.0f / (1.0f + gH * (gH + kH));
        const float ah2     = gH * ah1;
        const float ah3     = gH * ah2;

        const float drvGain = 1.0f + 2.8f * p.filtDrive;
        const float drvNorm = 1.0f / (1.0f + 0.55f * p.filtDrive);
        const float baseOct = p.filtCut * 9.57f;
        const float keyOct  = ((float) midiNote - 60.0f) * (1.0f / 12.0f) * p.filtKey;
        const float velScale = 1.0f - p.filtVel + p.filtVel * vel;
        const float envDepth = p.filtEnvAmt * 5.0f * velScale;
        const float lfoMod   = lfoBi * p.filtLfo * 2.5f;
        const float res      = clampf (p.filtRes, 0.0f, 1.0f);
        const int   mode     = clampi (p.filtMode, 0, 3);

        for (int off = 0; off < n; off += 16)
        {
            const int end  = std::min (n, off + 16);
            const int subN = end - off;
            stepEnvSubBlock (subN, sr, p.filtAtt, p.filtDec, p.filtSus, p.filtRel);

            const float envMod = env * envDepth;
            const float fc = clampf (25.0f * std::exp2f (clampf (baseOct + keyOct + envMod + lfoMod, -1.0f, 9.65f)),
                                     20.0f, sr * 0.44f);
            const float g = std::tan (kPi * fc * invSr);

            if (mode == 1) // 24 dB/oct 2-stage Cascaded TPT SVF Lowpass with warm OTA resonance
            {
                const float k1  = 2.0f - 1.72f * res;
                const float a1  = 1.0f / (1.0f + g * (g + k1));
                const float a2  = g * a1;
                const float a3  = g * a2;
                const float k2  = 2.0f - 1.65f * res;
                const float b1  = 1.0f / (1.0f + g * (g + k2));
                const float b2  = g * b1;
                const float b3  = g * b2;
                const float resComp = 1.0f + 0.35f * res;

                for (int i = off; i < end; ++i)
                {
                    float x = buf[i];
                    if (doHpf)
                    {
                        const float v3h = x - hp2;
                        const float v1h = ah1 * hp1 + ah2 * v3h;
                        const float v2h = hp2 + ah2 * hp1 + ah3 * v3h;
                        hp1 = 2.0f * v1h - hp1;
                        hp2 = 2.0f * v2h - hp2;
                        x = x - kH * v1h - v2h;
                    }
                    if (doDrive) x = fastTanh (x * drvGain) * drvNorm;
                    x *= resComp;

                    const float s1  = fastTanh (ic1 * 0.85f) * (1.0f / 0.85f);
                    const float v3a = x - ic2;
                    const float v1a = a1 * s1 + a2 * v3a;
                    const float v2a = ic2 + a2 * s1 + a3 * v3a;
                    ic1 = 2.0f * v1a - s1;
                    ic2 = 2.0f * v2a - ic2;

                    const float s3  = fastTanh (ic3 * 0.85f) * (1.0f / 0.85f);
                    const float v3b = v2a - ic4;
                    const float v1b = b1 * s3 + b2 * v3b;
                    const float v2b = ic4 + b2 * s3 + b3 * v3b;
                    ic3 = 2.0f * v1b - s3;
                    ic4 = 2.0f * v2b - ic4;

                    buf[i] = v2b;
                }
            }
            else // 12 dB/oct TPT SVF: 0=LP12, 2=BP12, 3=HP12
            {
                const float k  = 2.0f - 1.94f * res;
                const float a1 = 1.0f / (1.0f + g * (g + k));
                const float a2 = g * a1;
                const float a3 = g * a2;

                for (int i = off; i < end; ++i)
                {
                    float x = buf[i];
                    if (doHpf)
                    {
                        const float v3h = x - hp2;
                        const float v1h = ah1 * hp1 + ah2 * v3h;
                        const float v2h = hp2 + ah2 * hp1 + ah3 * v3h;
                        hp1 = 2.0f * v1h - hp1;
                        hp2 = 2.0f * v2h - hp2;
                        x = x - kH * v1h - v2h;
                    }
                    if (doDrive) x = fastTanh (x * drvGain) * drvNorm;

                    const float s1 = fastTanh (ic1 * 0.85f) * (1.0f / 0.85f);
                    const float v3 = x - ic2;
                    const float v1 = a1 * s1 + a2 * v3;
                    const float v2 = ic2 + a2 * s1 + a3 * v3;
                    ic1 = 2.0f * v1 - s1;
                    ic2 = 2.0f * v2 - ic2;

                    if (mode == 0)      buf[i] = v2;
                    else if (mode == 2) buf[i] = v1;
                    else                buf[i] = x - k * v1 - v2;
                }
            }
        }

        if (std::fabs (hp1) < 1e-24f) hp1 = 0.0f;
        if (std::fabs (hp2) < 1e-24f) hp2 = 0.0f;
        if (std::fabs (ic1) < 1e-24f) ic1 = 0.0f;
        if (std::fabs (ic2) < 1e-24f) ic2 = 0.0f;
        if (std::fabs (ic3) < 1e-24f) ic3 = 0.0f;
        if (std::fabs (ic4) < 1e-24f) ic4 = 0.0f;
    }
};

// Authentic SN76489-style PSG Sub-Chip voice (square / sub / narrow pulse / 15-bit LFSR noise)
struct PsgVoice
{
    float phase = 0.0f;
    float env   = 0.0f;
    bool  gate  = false;
    float hz    = 220.0f;
    uint16_t lfsr = 0x4000u;
    float noiseOut = 1.0f;
    float noiseCnt = 0.0f;

    void reset() { phase = 0.0f; env = 0.0f; gate = false; lfsr = 0x4000u; noiseOut = 1.0f; noiseCnt = 0.0f; }

    inline float tick (float invSr, int wave, float decCoef)
    {
        if (gate) env += (1.0f - env) * 0.04f;
        else      env *= decCoef;
        if (env < 1e-4f) { env = 0.0f; return 0.0f; }

        if (wave == 4)
        {
            const float step = clampf (hz * 12.0f * invSr, 0.002f, 0.5f);
            noiseCnt += step;
            if (noiseCnt >= 1.0f)
            {
                noiseCnt -= 1.0f;
                const uint16_t bit = ((lfsr & 1u) ^ ((lfsr >> 1) & 1u)) & 1u;
                lfsr = (uint16_t) ((lfsr >> 1) | (bit << 14));
                noiseOut = (lfsr & 1u) ? 0.65f : -0.65f;
            }
            return noiseOut * env;
        }

        const float mult = (wave == 1) ? 0.5f : (wave == 2 ? 0.25f : 1.0f);
        const float pw   = (wave == 3) ? 0.22f : 0.50f;
        const float dt   = clampf (hz * mult * invSr, 0.0002f, 0.45f);
        const float y    = phase < pw ? 0.65f : -0.65f;
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;
        return y * env;
    }
};

// Lightweight 80s BBD Stereo Chorus + Tempo-Synced Ping-Pong Stereo Delay + MD1 Filter + DAC Grit
class ArcadeFx
{
public:
    static constexpr int kChBuf  = 2048;
    static constexpr int kChMask = kChBuf - 1;
    static constexpr int kDlyMax = 96000;

    void prepare (float sr)
    {
        sr_ = sr > 8000.0f ? sr : 44100.0f;
        md1Coef_ = 1.0f - std::exp (-kTwoPi * 3400.0f / sr_);
        reset();
    }

    void reset()
    {
        chL_.fill (0.0f); chR_.fill (0.0f); chPos_ = 0; chPh_ = 0.0f;
        dlyL_.fill (0.0f); dlyR_.fill (0.0f); dlyPos_ = 0; dlyTail_ = 0;
        dlyLpL_ = dlyLpR_ = 0.0f;
        md1L1_ = md1L2_ = md1R1_ = md1R2_ = 0.0f;
        dcX1L_ = dcY1L_ = dcX1R_ = dcY1R_ = 0.0f;
        crushCnt_ = 0.0f; holdL_ = holdR_ = 0.0f;
    }

    inline void processBlock (float* bufL, float* bufR, int n, int pcbTone, float dacGrit,
                              float chorusMix, float delayMix, int delayDiv,
                              float delayFb, float delayTone, double bpm)
    {
        const float grit = (pcbTone == 2) ? std::max (0.45f, dacGrit) : dacGrit;
        const bool  doGrit   = grit > 0.01f;
        const float gStep    = 1.0f + grit * 4.5f;
        const float gLevels  = doGrit ? (512.0f * std::pow (0.06f, grit)) : 512.0f;
        const float invGLev  = 1.0f / gLevels;
        const bool  doMd1    = (pcbTone == 1);
        const bool  doChorus = chorusMix > 0.005f;

        static constexpr float kDivBeats[6] = { 0.25f, 1.0f/3.0f, 0.5f, 0.75f, 1.0f, 2.0f };
        const float beats = kDivBeats[clampi (delayDiv, 0, 5)];
        const float sec   = (float) (60.0 / std::max (40.0, std::min (280.0, bpm))) * beats;
        const int   dSmp  = clampi ((int) (sec * sr_), 64, kDlyMax - 2);
        const float lpC   = 0.06f + 0.75f * clampf (delayTone, 0.0f, 1.0f);
        const float fb    = clampf (delayFb, 0.0f, 0.88f);
        const float send  = clampf (delayMix, 0.0f, 1.0f);
        if (send > 0.002f)
            dlyTail_ = std::max (dlyTail_, dSmp + 2048);
        const bool  doDelay = (send > 0.002f || dlyTail_ > 0);
        float       dlyPeak = 0.0f;
        const float ms    = sr_ * 0.001f;
        const float chInc = 0.68f / sr_;

        for (int i = 0; i < n; ++i)
        {
            float l = bufL[i];
            float r = bufR[i];

            if (doGrit)
            {
                crushCnt_ += 1.0f;
                if (crushCnt_ >= gStep)
                {
                    crushCnt_ -= gStep;
                    holdL_ = std::round (l * gLevels) * invGLev;
                    holdR_ = std::round (r * gLevels) * invGLev;
                }
                l = holdL_;
                r = holdR_;
            }

            if (doMd1)
            {
                md1L1_ += (l - md1L1_) * md1Coef_; md1L2_ += (md1L1_ - md1L2_) * md1Coef_;
                md1R1_ += (r - md1R1_) * md1Coef_; md1R2_ += (md1R1_ - md1R2_) * md1Coef_;
                l = md1L2_ * 1.18f;
                r = md1R2_ * 1.18f;
            }

            chL_[(size_t) chPos_] = l;
            chR_[(size_t) chPos_] = r;
            if (doChorus)
            {
                chPh_ += chInc; if (chPh_ >= 1.0f) chPh_ -= 1.0f;
                // Fast triangle-smoothed LFO for zero per-sample sinf()!
                const float t1 = 4.0f * std::fabs (chPh_ - 0.5f) - 1.0f;
                float p2 = chPh_ + 0.3333f; if (p2 >= 1.0f) p2 -= 1.0f;
                const float t2 = 4.0f * std::fabs (p2 - 0.5f) - 1.0f;
                const float cL = readCh (chL_, (8.5f + 2.8f * t1) * ms);
                const float cR = readCh (chR_, (9.2f + 2.8f * t2) * ms);
                l = l * (1.0f - 0.30f * chorusMix) + cL * (0.68f * chorusMix);
                r = r * (1.0f - 0.30f * chorusMix) + cR * (0.68f * chorusMix);
            }
            chPos_ = (chPos_ + 1) & kChMask;

            if (doDelay)
            {
                int rPos = dlyPos_ - dSmp;
                if (rPos < 0) rPos += kDlyMax;
                const float tapL = dlyL_[(size_t) rPos];
                const float tapR = dlyR_[(size_t) rPos];
                dlyLpL_ += (tapL - dlyLpL_) * lpC;
                dlyLpR_ += (tapR - dlyLpR_) * lpC;
                if (std::fabs (dlyLpL_) < 1e-24f) dlyLpL_ = 0.0f;
                if (std::fabs (dlyLpR_) < 1e-24f) dlyLpR_ = 0.0f;
                const float wL = l * send + dlyLpR_ * fb;
                const float wR = r * send + dlyLpL_ * fb;
                dlyL_[(size_t) dlyPos_] = wL;
                dlyR_[(size_t) dlyPos_] = wR;
                const float aL = std::fabs (dlyLpL_) + std::fabs (wL);
                const float aR = std::fabs (dlyLpR_) + std::fabs (wR);
                if (aL > dlyPeak) dlyPeak = aL;
                if (aR > dlyPeak) dlyPeak = aR;
                l += dlyLpL_ * 0.75f;
                r += dlyLpR_ * 0.75f;
            }
            else
            {
                dlyL_[(size_t) dlyPos_] = 0.0f;
                dlyR_[(size_t) dlyPos_] = 0.0f;
            }
            if (++dlyPos_ >= kDlyMax) dlyPos_ = 0;

            const float yL = l - dcX1L_ + 0.996f * dcY1L_; dcX1L_ = l; dcY1L_ = yL;
            const float yR = r - dcX1R_ + 0.996f * dcY1R_; dcX1R_ = r; dcY1R_ = yR;
            l = std::fabs (yL) > 0.88f ? (yL > 0 ? 1.0f : -1.0f) * (0.88f + 0.12f * fastTanh ((std::fabs (yL) - 0.88f) * 4.0f)) : yL;
            r = std::fabs (yR) > 0.88f ? (yR > 0 ? 1.0f : -1.0f) * (0.88f + 0.12f * fastTanh ((std::fabs (yR) - 0.88f) * 4.0f)) : yR;
            if (std::fabs (l) < 1e-24f) l = 0.0f;
            if (std::fabs (r) < 1e-24f) r = 0.0f;

            bufL[i] = l;
            bufR[i] = r;
        }

        if (doDelay && send <= 0.002f)
        {
            if (dlyPeak > 1e-5f)
                dlyTail_ = std::max (dlyTail_, dSmp + 2048);
            else
            {
                dlyTail_ = std::max (0, dlyTail_ - n);
                if (dlyTail_ == 0)
                {
                    dlyLpL_ = 0.0f;
                    dlyLpR_ = 0.0f;
                }
            }
        }
    }

private:
    inline float readCh (const std::array<float, kChBuf>& b, float d) const
    {
        const float dc = clampf (d, 2.0f, (float) (kChBuf - 4));
        const int di = (int) dc;
        const float frac = dc - (float) di;
        const int i0 = (chPos_ - di) & kChMask;
        const int i1 = (i0 - 1) & kChMask;
        return b[(size_t) i0] + frac * (b[(size_t) i1] - b[(size_t) i0]);
    }

    float sr_ = 44100.0f, md1Coef_ = 0.35f;
    std::array<float, kChBuf>  chL_ {}, chR_ {};
    int   chPos_ = 0;
    float chPh_ = 0.0f;
    std::array<float, kDlyMax> dlyL_ {}, dlyR_ {};
    int   dlyPos_ = 0, dlyTail_ = 0;
    float dlyLpL_ = 0.0f, dlyLpR_ = 0.0f;
    float md1L1_ = 0.0f, md1L2_ = 0.0f, md1R1_ = 0.0f, md1R2_ = 0.0f;
    float dcX1L_ = 0.0f, dcY1L_ = 0.0f, dcX1R_ = 0.0f, dcY1R_ = 0.0f;
    float crushCnt_ = 0.0f, holdL_ = 0.0f, holdR_ = 0.0f;
};

class OpArcadeEngine
{
public:
    struct VoiceSlot
    {
        bool     active = false;
        bool     held   = false;
        bool     sustained = false;
        int      note   = 60;
        float    vel    = 1.0f;
        double   curMidi = 60.0;
        double   tgtMidi = 60.0;
        float    panDetuneSemis = 0.0f;
        float    panL = 1.0f, panR = 1.0f;
        uint64_t age  = 0;
        Ym4OpVoice opmV;
        Ym4OpVoice opn2V;
        PsgVoice   psg;
        HybridVoiceFilter vcf;
    };

    void prepare (double sampleRate)
    {
        sr_    = sampleRate > 8000.0 ? sampleRate : 44100.0;
        invSr_ = 1.0f / (float) sr_;
        fx_.prepare ((float) sr_);
        resetChips();
    }

    void resetChips()
    {
        bendSemis_ = 0.0f;
        modWheel_ = 0.0f;
        sustainPedal_ = false;
        ageCounter_ = 0;
        lfoPhase_ = 0.0f;
        lfoRndVal_ = 0.0f;
        opmLfsr_ = 0x1FFFFu;
        opmNoiseCnt_ = 0.0f;
        opmNoiseSign_ = 1;
        outPeakL_ = outPeakR_ = 0.0f;
        activeVoices_ = 0;
        for (auto& v : voices_)
        {
            v = VoiceSlot {};
            v.opmV.reset();
            v.opn2V.reset();
            v.psg.reset();
            v.vcf.reset();
        }
        fx_.reset();
    }

    void allNotesOff()
    {
        sustainPedal_ = false;
        for (int ch = 0; ch < kMaxVoices; ++ch)
        {
            voices_[ch].held = false;
            voices_[ch].sustained = false;
            voices_[ch].psg.gate = false;
            voices_[ch].vcf.noteOff();
            for (int op = 0; op < 4; ++op)
            {
                voices_[ch].opmV.ops[op].keyOff();
                voices_[ch].opn2V.ops[op].keyOff();
            }
        }
    }

    void setPitchBend (float normMinus1To1) { bendSemis_ = normMinus1To1; }
    void setModWheel  (float norm0To1)      { modWheel_  = clampf (norm0To1, 0.0f, 1.0f); }
    void setSustain   (bool down)
    {
        sustainPedal_ = down;
        if (! down)
        {
            for (int ch = 0; ch < kMaxVoices; ++ch)
            {
                if (voices_[ch].sustained && ! voices_[ch].held)
                {
                    voices_[ch].sustained = false;
                    voices_[ch].psg.gate = false;
                    voices_[ch].vcf.noteOff();
                    for (int op = 0; op < 4; ++op)
                    {
                        voices_[ch].opmV.ops[op].keyOff();
                        voices_[ch].opn2V.ops[op].keyOff();
                    }
                }
            }
        }
    }

    void noteOn (int midiNote, float velocity, const SynthParams& p)
    {
        const int maxV = maxVoicesForMode (p.chipMode);
        const float vel = clampf (velocity, 0.05f, 1.0f);

        auto triggerSlot = [&] (int ch, float detSemi, float pL, float pR, bool legato) {
            auto& v = voices_[ch];
            const bool wasInactive = ! v.active;
            v.note = midiNote; v.vel = vel;
            v.tgtMidi = (double) midiNote;
            if (! legato) v.curMidi = (double) midiNote;
            v.held = true; v.active = true; v.sustained = false;
            v.panDetuneSemis = detSemi;
            v.panL = pL; v.panR = pR;
            v.age = ++ageCounter_;
            v.psg.gate = true;
            if (! legato)
            {
                v.opmV.fb0 = v.opmV.fb1 = 0;
                v.opn2V.fb0 = v.opn2V.fb1 = 0;
                if (wasInactive) v.vcf.reset();
                v.vcf.noteOn();
                for (int op = 0; op < 4; ++op)
                {
                    const int ar = clampi (p.op[op].ar - (int) std::lround (p.macroAtt * 14.0f), 1, 31);
                    v.opmV.ops[op].keyOn (ar);
                    v.opn2V.ops[op].keyOn (ar);
                }
            }
        };

        if (p.voiceMode == 1) // Mono / Legato
        {
            const bool legato = voices_[0].held && p.porta > 0.01f;
            triggerSlot (0, 0.0f, 1.0f, 1.0f, legato);
            return;
        }

        if (p.voiceMode == 2) // Unison 2 (paired stereo voices)
        {
            const int pairs = maxV / 2;
            int chosenPair = 0;
            uint64_t oldest = UINT64_MAX;
            for (int pr = 0; pr < pairs; ++pr)
            {
                const int c0 = pr * 2;
                if (! voices_[c0].held && ! voices_[c0].sustained) { chosenPair = pr; break; }
                if (voices_[c0].age < oldest) { oldest = voices_[c0].age; chosenPair = pr; }
            }
            const float detSemi = 0.04f + 0.30f * p.detune;
            const float wide = 0.40f + 0.55f * p.detune;
            triggerSlot (chosenPair * 2 + 0, -detSemi, 1.0f + wide * 0.4f, 1.0f - wide * 0.4f, false);
            triggerSlot (chosenPair * 2 + 1, +detSemi, 1.0f - wide * 0.4f, 1.0f + wide * 0.4f, false);
            return;
        }

        // Standard Polyphonic voice allocation
        int ch = -1;
        for (int i = 0; i < maxV; ++i)
            if (voices_[i].active && voices_[i].note == midiNote) { ch = i; break; }
        if (ch < 0)
        {
            uint64_t oldest = UINT64_MAX;
            for (int i = 0; i < maxV; ++i)
            {
                if (! voices_[i].held && ! voices_[i].sustained && voices_[i].age < oldest)
                {
                    oldest = voices_[i].age; ch = i;
                }
            }
        }
        if (ch < 0)
        {
            uint64_t oldest = UINT64_MAX;
            for (int i = 0; i < maxV; ++i)
                if (voices_[i].age < oldest) { oldest = voices_[i].age; ch = i; }
        }
        const float sign = (ch & 1) ? 1.0f : -1.0f;
        const float det = sign * (0.015f + 0.12f * p.detune) * (float) (1 + (ch % 3)) * 0.45f;
        const float pan = sign * 0.22f * p.detune;
        triggerSlot (ch, det, 1.0f - pan, 1.0f + pan, false);
    }

    void noteOff (int midiNote)
    {
        for (int ch = 0; ch < kMaxVoices; ++ch)
        {
            if (voices_[ch].held && voices_[ch].note == midiNote)
            {
                voices_[ch].held = false;
                if (sustainPedal_)
                {
                    voices_[ch].sustained = true;
                }
                else
                {
                    voices_[ch].psg.gate = false;
                    voices_[ch].vcf.noteOff();
                    for (int op = 0; op < 4; ++op)
                    {
                        voices_[ch].opmV.ops[op].keyOff();
                        voices_[ch].opn2V.ops[op].keyOff();
                    }
                }
            }
        }
    }

    void render (float* outL, float* outR, int numFrames, const SynthParams& p, double bpm)
    {
        if (numFrames <= 0 || ! outL || ! outR) return;
        std::memset (outL, 0, (size_t) numFrames * sizeof (float));
        std::memset (outR, 0, (size_t) numFrames * sizeof (float));

        const int  maxV    = maxVoicesForMode (p.chipMode);
        const bool runOpm  = (p.chipMode == 0 || p.chipMode == 2);
        const bool runOpn2 = (p.chipMode == 1 || p.chipMode == 2);
        const int  alg     = clampi (p.alg, 0, 7);
        const int  fb      = clampi (p.feedback, 0, 7);

        float gOpm = 0.0f, gOpn2 = 0.0f;
        if (p.chipMode == 0)      { gOpm  = 1.0f; }
        else if (p.chipMode == 1) { gOpn2 = 1.0f; }
        else
        {
            const float b = clampf (p.chipBlend, 0.0f, 1.0f);
            gOpm  = std::sqrt (1.0f - b) * 1.05f;
            gOpn2 = std::sqrt (b)        * 1.05f;
        }

        // Precompute block-rate operator EG & TL parameters (zero powf/expf inside per-sample loop!)
        float arInc[4], d1Inc[4], d2Inc[4], rrInc[4], slTgt[4], mulRatio[4], dt1Cents[4];
        int   baseTl[4], ssgMode[4], opAm[4], opDt2[4];
        bool  carFlags[4];

        for (int op = 0; op < 4; ++op)
        {
            const auto& o = p.op[op];
            carFlags[op] = isCarrierOp (alg, op);
            mulRatio[op] = (o.mul <= 0) ? 0.5f : (float) o.mul;
            dt1Cents[op] = (float) clampi (o.dt1, -3, 3) * 3.8f; // authentic Yamaha DT1 fine detune
            opDt2[op]    = clampi (o.dt2, 0, 3);
            opAm[op]     = o.am;

            int tl = (o.level <= 0) ? 127 : clampi ((int) std::lround ((1.0f - (float) o.level * 0.01f) * 76.0f), 0, 127);
            if (! carFlags[op])
                tl = clampi (tl + (int) std::lround (-p.fmBite * 34.0f), 0, 127);
            baseTl[op] = tl;

            const int ar  = clampi (o.ar  - (int) std::lround (p.macroAtt * 14.0f), 1, 31);
            const int d1r = clampi (o.d1r - (int) std::lround (p.macroDec * 12.0f), 0, 31);
            const int d2r = (d1r >= 10) ? (d1r / 4) : 0;
            const int rr  = clampi (o.rr  - (int) std::lround (p.macroRel * 7.0f),  1, 15);
            const int sl  = clampi (o.sl, 0, 15);

            arInc[op]   = ymRateToAttackScale (ar, invSr_);
            d1Inc[op]   = ymRateToDecayInc (d1r, invSr_);
            d2Inc[op]   = ymRateToDecayInc (d2r, invSr_);
            rrInc[op]   = ymRateToDecayInc (rr * 2 + 1, invSr_);
            slTgt[op]   = (sl >= 15) ? 1023.0f : (float) (sl * 32); // exact Yamaha 3 dB/step sustain level!
            ssgMode[op] = carFlags[op] ? clampi (p.ssgEg, 0, 3) : 0;
        }

        // Advance global LFO once per block (~0.2..18 Hz) for silky PM vibrato & AM tremolo
        const float lfoHz = 0.25f + 16.0f * p.lfoSpeed * p.lfoSpeed;
        lfoPhase_ += lfoHz * (float) numFrames * invSr_;
        if (lfoPhase_ >= 1.0f)
        {
            lfoPhase_ -= std::floor (lfoPhase_);
            opmLfsr_ ^= opmLfsr_ << 13; opmLfsr_ ^= opmLfsr_ >> 17; opmLfsr_ ^= opmLfsr_ << 5;
            lfoRndVal_ = (float) (int32_t) opmLfsr_ * (1.0f / 2147483648.0f);
        }
        float lfoBi = 0.0f;
        if (p.lfoWave == 0)      lfoBi = 1.0f - 2.0f * lfoPhase_;
        else if (p.lfoWave == 1) lfoBi = (lfoPhase_ < 0.5f) ? 1.0f : -1.0f;
        else if (p.lfoWave == 2) lfoBi = 4.0f * std::fabs (lfoPhase_ - 0.5f) - 1.0f;
        else                     lfoBi = lfoRndVal_;

        const float pmEff = clampf (p.lfoPm + modWheel_ * 0.75f, 0.0f, 1.0f);
        const float pmSemis = lfoBi * pmEff * 0.65f;
        const uint32_t amAttenVal = (uint32_t) clampi ((int) std::lround ((0.5f - 0.5f * lfoBi) * p.lfoAm * 220.0f), 0, 320);
        uint32_t amAtten[4] = {
            opAm[0] ? amAttenVal : 0u, opAm[1] ? amAttenVal : 0u,
            opAm[2] ? amAttenVal : 0u, opAm[3] ? amAttenVal : 0u
        };

        const float portaCoef = p.porta > 0.005f
            ? (1.0f - std::exp (-(float) numFrames / ((0.008f + 0.45f * p.porta * p.porta) * (float) sr_)))
            : 1.0f;
        const float psgDecCoef = std::exp (-1.0f / ((0.015f + 1.4f * p.psgDecay * p.psgDecay) * (float) sr_));

        constexpr float kYmNorm = 1.0f / 6800.0f; // hot hardware level (~-4 dBFS single note, clean chord headroom!)
        const float scaleOpm  = gOpm  * kYmNorm;
        const float scaleOpn2 = gOpn2 * kYmNorm;
        const float noiseStep = p.opmNoise > 0 ? clampf ((float) (1 + p.opmNoise) * 520.0f * invSr_, 0.01f, 0.95f) : 0.0f;
        const bool  needFilter = (p.filtMode == 2 || p.filtMode == 3)
                              || p.filtCut < 0.995f || p.filtRes > 0.005f || p.filtHpf > 0.005f
                              || p.filtDrive > 0.005f || std::fabs (p.filtEnvAmt) > 0.005f || p.filtLfo > 0.005f;

        const double phaseScale = 4294967296.0 / sr_; // Hz -> 32-bit phase increment!
        float vBuf[128];

        for (int blkOff = 0; blkOff < numFrames; blkOff += 128)
        {
            const int chunk = std::min (128, numFrames - blkOff);
            float* cOutL = outL + blkOff;
            float* cOutR = outR + blkOff;

            for (int ch = 0; ch < maxV; ++ch)
            {
                auto& v = voices_[ch];
                if (! v.active) continue;

                const bool opmSilent  = ! runOpm  || v.opmV.isSilent();
                const bool opn2Silent = ! runOpn2 || v.opn2V.isSilent();
                const bool psgSilent  = (p.psgMix <= 0.005f) || (! v.psg.gate && v.psg.env <= 1e-4f);
                const bool vcfSilent  = ! needFilter || v.vcf.isQuiet();
                if (opmSilent && opn2Silent && psgSilent && vcfSilent)
                {
                    if (! v.held && ! v.sustained) { v.active = false; v.vcf.reset(); }
                    continue;
                }

                // Smooth portamento glide
                if (blkOff == 0)
                    v.curMidi += (v.tgtMidi - v.curMidi) * portaCoef;

                const double baseMidi = v.curMidi + (double) v.panDetuneSemis
                                      + (double) (bendSemis_ * (float) p.bendRange) + (double) pmSemis;
                const double baseHzOpm = 440.0 * std::pow (2.0, (baseMidi - 69.0) / 12.0);
                v.psg.hz = (float) baseHzOpm;

                // Velocity scaling on carrier TL (plus subtle 1/3 scaling on modulators for expressive FM bite!)
                const int velTl = (int) std::lround (((1.0f - v.vel) * 26.0f * p.velSens) / 0.75f);
                uint32_t tl10[4];
                for (int op = 0; op < 4; ++op)
                {
                    const int tl = clampi (baseTl[op] + (carFlags[op] ? velTl : (velTl / 3)), 0, 127);
                    tl10[op] = (uint32_t) (tl << 3);
                }

                if (needFilter)
                    std::memset (vBuf, 0, (size_t) chunk * sizeof (float));

                // OPM (YM2151) voice pass
                if (! opmSilent)
                {
                    uint32_t incOpm[4];
                    for (int op = 0; op < 4; ++op)
                    {
                        const double opHz = baseHzOpm * (double) mulRatio[op] * (double) kOpmDt2Ratio[opDt2[op]]
                                          * std::pow (2.0, (double) dt1Cents[op] / 1200.0);
                        incOpm[op] = (uint32_t) std::min (opHz * phaseScale, 1.9e9);
                    }
                    const bool useNoise = (p.opmNoise > 0) && (ch == maxV - 1 || ch == 0);
                    const int noSsg[4] = { 0, 0, 0, 0 };

                    if (needFilter)
                    {
                        for (int i = 0; i < chunk; ++i)
                        {
                            if (useNoise)
                            {
                                opmNoiseCnt_ += noiseStep;
                                if (opmNoiseCnt_ >= 1.0f)
                                {
                                    opmNoiseCnt_ -= 1.0f;
                                    const uint32_t b = ((opmLfsr_ & 1u) ^ ((opmLfsr_ >> 3) & 1u)) & 1u;
                                    opmLfsr_ = (opmLfsr_ >> 1) | (b << 16);
                                    opmNoiseSign_ = (opmLfsr_ & 1u) ? 1 : 0;
                                }
                            }
                            const int32_t s = v.opmV.tick (alg, fb, incOpm, tl10, arInc, d1Inc, d2Inc, rrInc, slTgt,
                                                           amAtten, noSsg, useNoise, opmNoiseSign_, false);
                            vBuf[i] += (float) s * scaleOpm;
                        }
                    }
                    else
                    {
                        const float pL = v.panL * scaleOpm;
                        const float pR = v.panR * scaleOpm;
                        for (int i = 0; i < chunk; ++i)
                        {
                            if (useNoise)
                            {
                                opmNoiseCnt_ += noiseStep;
                                if (opmNoiseCnt_ >= 1.0f)
                                {
                                    opmNoiseCnt_ -= 1.0f;
                                    const uint32_t b = ((opmLfsr_ & 1u) ^ ((opmLfsr_ >> 3) & 1u)) & 1u;
                                    opmLfsr_ = (opmLfsr_ >> 1) | (b << 16);
                                    opmNoiseSign_ = (opmLfsr_ & 1u) ? 1 : 0;
                                }
                            }
                            const int32_t s = v.opmV.tick (alg, fb, incOpm, tl10, arInc, d1Inc, d2Inc, rrInc, slTgt,
                                                           amAtten, noSsg, useNoise, opmNoiseSign_, false);
                            const float sf = (float) s;
                            cOutL[i] += sf * pL;
                            cOutR[i] += sf * pR;
                        }
                    }
                }

                // OPN2 (YM2612) voice pass (with 9-bit ladder DAC + SSG-EG + Hybrid octave/detune offset!)
                if (! opn2Silent && ch < 6)
                {
                    const double octOff = (p.chipMode == 2) ? (double) (p.opn2Oct * 12) : 0.0;
                    const double detOff = (p.chipMode == 2) ? (double) (p.opn2Detune * 0.5f) : 0.0;
                    const double baseHzOpn2 = 440.0 * std::pow (2.0, (baseMidi + octOff + detOff - 69.0) / 12.0);
                    uint32_t incOpn2[4];
                    for (int op = 0; op < 4; ++op)
                    {
                        const double opHz = baseHzOpn2 * (double) mulRatio[op]
                                          * std::pow (2.0, (double) dt1Cents[op] / 1200.0);
                        incOpn2[op] = (uint32_t) std::min (opHz * phaseScale, 1.9e9);
                    }

                    if (needFilter)
                    {
                        for (int i = 0; i < chunk; ++i)
                        {
                            const int32_t s = v.opn2V.tick (alg, fb, incOpn2, tl10, arInc, d1Inc, d2Inc, rrInc, slTgt,
                                                            amAtten, ssgMode, false, 0, true);
                            vBuf[i] += (float) s * scaleOpn2;
                        }
                    }
                    else
                    {
                        const float pL = v.panL * scaleOpn2;
                        const float pR = v.panR * scaleOpn2;
                        for (int i = 0; i < chunk; ++i)
                        {
                            const int32_t s = v.opn2V.tick (alg, fb, incOpn2, tl10, arInc, d1Inc, d2Inc, rrInc, slTgt,
                                                            amAtten, ssgMode, false, 0, true);
                            const float sf = (float) s;
                            cOutL[i] += sf * pL;
                            cOutR[i] += sf * pR;
                        }
                    }
                }

                // SN76489 PSG Sub-Chip pass
                if (! psgSilent)
                {
                    const float psgG = 0.28f * p.psgMix * (0.4f + 0.6f * v.vel);
                    if (needFilter)
                    {
                        for (int i = 0; i < chunk; ++i)
                            vBuf[i] += v.psg.tick (invSr_, p.psgWave, psgDecCoef) * psgG;
                    }
                    else
                    {
                        for (int i = 0; i < chunk; ++i)
                        {
                            const float ps = v.psg.tick (invSr_, p.psgWave, psgDecCoef) * psgG;
                            cOutL[i] += ps;
                            cOutR[i] += ps;
                        }
                    }
                }

                if (needFilter)
                {
                    v.vcf.processMonoBlock (vBuf, chunk, (float) sr_, invSr_, v.note, v.vel, lfoBi, p);
                    const float pL = v.panL;
                    const float pR = v.panR;
                    for (int i = 0; i < chunk; ++i)
                    {
                        cOutL[i] += vBuf[i] * pL;
                        cOutR[i] += vBuf[i] * pR;
                    }
                }
            }
        }

        int heldCount = 0;
        for (int ch = 0; ch < maxV; ++ch)
            if (voices_[ch].held || voices_[ch].sustained) ++heldCount;
        activeVoices_ = heldCount;

        // Master volume + PCB Tone / DAC Grit / 80s Chorus / Tempo-Synced Delay + Soft Limiter
        const float vol = clampf (p.volume, 0.0f, 1.0f) * 1.15f;
        for (int i = 0; i < numFrames; ++i)
        {
            outL[i] *= vol;
            outR[i] *= vol;
        }
        fx_.processBlock (outL, outR, numFrames, p.pcbTone, p.dacGrit,
                          p.chorusMix, p.delayMix, p.delayDiv, p.delayFb, p.delayTone, bpm);

        float pkL = 0.0f, pkR = 0.0f;
        for (int i = 0; i < numFrames; ++i)
        {
            const float aL = std::fabs (outL[i]), aR = std::fabs (outR[i]);
            if (aL > pkL) pkL = aL;
            if (aR > pkR) pkR = aR;
        }
        outPeakL_ = std::max (pkL, outPeakL_ * 0.78f);
        outPeakR_ = std::max (pkR, outPeakR_ * 0.78f);
    }

    float peakL() const { return outPeakL_; }
    float peakR() const { return outPeakR_; }
    int   activeVoices() const { return activeVoices_; }

private:
    static inline int maxVoicesForMode (int chipMode) { return (chipMode == 0) ? 8 : 6; }

    double sr_ = 44100.0;
    float  invSr_ = 1.0f / 44100.0f;
    std::array<VoiceSlot, kMaxVoices> voices_ {};
    uint64_t ageCounter_ = 0;
    float    bendSemis_ = 0.0f, modWheel_ = 0.0f;
    bool     sustainPedal_ = false;
    float    lfoPhase_ = 0.0f, lfoRndVal_ = 0.0f;
    uint32_t opmLfsr_ = 0x1FFFFu;
    float    opmNoiseCnt_ = 0.0f;
    int32_t  opmNoiseSign_ = 1;
    ArcadeFx fx_;
    float    outPeakL_ = 0.0f, outPeakR_ = 0.0f;
    int      activeVoices_ = 0;
};

} // namespace oparcade
