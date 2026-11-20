// OP-ARCADE — Silicon Chip Pitch Accuracy, Hybrid Layering, FM Bite, PSG & 32-Preset Validation Suite.
#include "../bridge/OpArcadePresets.h"
#include "../dsp/OpArcadeEngine.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace oparcade;

static int gPass = 0, gFail = 0;
static void check (bool ok, const char* name, const char* detail = "")
{
    if (ok) { ++gPass; std::printf ("PASS  %-48s %s\n", name, detail); }
    else    { ++gFail; std::printf ("FAIL  %-48s %s\n", name, detail); }
}

static double rmsOf (const std::vector<float>& x, int start = 0)
{
    double s = 0.0;
    const int n = (int) x.size() - start;
    for (int i = start; i < (int) x.size(); ++i) s += (double) x[(size_t) i] * x[(size_t) i];
    return std::sqrt (s / std::max (1, n));
}

static double measureZeroCrossHz (const std::vector<float>& x, int start, int end, double sr)
{
    int crosses = 0;
    for (int i = start + 1; i < end; ++i)
        if (x[(size_t) (i - 1)] <= 0.0f && x[(size_t) i] > 0.0f) ++crosses;
    return (double) crosses * sr / (double) (end - start);
}

static bool allFinite (const std::vector<float>& x)
{
    for (float v : x) if (! std::isfinite (v)) return false;
    return true;
}

int main()
{
    const double sr = 44100.0;
    const int N = 22050; // 0.5 s
    char msg[160];

    std::printf ("=== OP-ARCADE Dual-Chip Silicon Validation Suite (sr=%.0f Hz) ===\n\n", sr);

    // 1. Exact A4 = 440.0 Hz tuning on BOTH OPM (YM2151) AND OPN2 (YM2612)!
    for (int chip = 0; chip <= 1; ++chip)
    {
        OpArcadeEngine eng;
        eng.prepare (sr);
        SynthParams p;
        p.chipMode = chip; p.alg = 0; p.feedback = 0; p.detune = 0.0f;
        p.lfoPm = 0.0f; p.lfoAm = 0.0f; p.chorusMix = 0.0f; p.delayMix = 0.0f; p.psgMix = 0.0f;
        p.op[0].level = 0; p.op[1].level = 0; p.op[2].level = 0;
        p.op[3] = { 1, 0, 0, 96, 31, 0, 0, 0, 7, 0 }; // pure sine carrier on OP4!

        eng.noteOn (69, 1.0f, p); // A4 = 440 Hz
        std::vector<float> L (N), R (N);
        for (int pos = 0; pos < N; pos += 128)
            eng.render (L.data() + pos, R.data() + pos, std::min (128, N - pos), p, 120.0);

        const double hz = measureZeroCrossHz (L, N / 4, N - 256, sr);
        const double rms = rmsOf (L, N / 4);
        std::snprintf (msg, sizeof (msg), "measured %.2f Hz (target 440.0 Hz), rms=%.3f", hz, rms);
        check (std::fabs (hz - 440.0) < 3.5 && rms > 0.10 && allFinite (L),
               chip == 0 ? "OPM (YM2151) A4 tuning & sine output"
                         : "OPN2 (YM2612) A4 tuning & sine output", msg);
    }

    // 2. Hybrid Dual-Chip Stacking (OPM + OPN2 together)
    {
        OpArcadeEngine eng;
        eng.prepare (sr);
        int cnt = 0;
        SynthParams p = factoryPresets (cnt)[0].p; // Outrun-ish Brass
        eng.noteOn (60, 1.0f, p); eng.noteOn (64, 1.0f, p); eng.noteOn (67, 1.0f, p);
        std::vector<float> L (N), R (N);
        for (int pos = 0; pos < N; pos += 128)
            eng.render (L.data() + pos, R.data() + pos, std::min (128, N - pos), p, 120.0);
        const double rL = rmsOf (L, 2000), rR = rmsOf (R, 2000);
        std::snprintf (msg, sizeof (msg), "rmsL=%.3f rmsR=%.3f voices=%d", rL, rR, eng.activeVoices());
        check (rL > 0.12 && rR > 0.12 && eng.activeVoices() == 3 && allFinite (L),
               "Hybrid Dual-Chip (YM2151 + YM2612) poly chord", msg);
    }

    // 3. FM BITE Macro measurably increases high-frequency harmonic richness without muting
    {
        int cnt = 0;
        SynthParams pSoft = factoryPresets (cnt)[0].p;
        pSoft.chorusMix = 0.0f; pSoft.delayMix = 0.0f; pSoft.fmBite = -0.8f;
        SynthParams pBite = pSoft; pBite.fmBite = +0.8f;

        auto renderDiff = [&] (const SynthParams& sp) {
            OpArcadeEngine eng; eng.prepare (sr);
            eng.noteOn (57, 1.0f, sp);
            std::vector<float> L (N), R (N);
            for (int pos = 0; pos < N; pos += 128)
                eng.render (L.data() + pos, R.data() + pos, std::min (128, N - pos), sp, 120.0);
            // Measure high-frequency slew (mean |x[i] - x[i-1]| / rms)
            double slew = 0.0;
            for (int i = 2001; i < N; ++i) slew += std::fabs (L[(size_t) i] - L[(size_t) (i - 1)]);
            return slew / (double) (N - 2001) / (rmsOf (L, 2000) + 1e-6);
        };
        const double sSoft = renderDiff (pSoft), sBite = renderDiff (pBite);
        std::snprintf (msg, sizeof (msg), "hfSlew soft=%.3f -> bite=%.3f", sSoft, sBite);
        check (sBite > sSoft * 1.4, "FM BITE macro sweeps modulator harmonic grit", msg);
    }

    // 4. PSG Sub-Chip & Mega Drive Model 1 Filter
    {
        OpArcadeEngine eng; eng.prepare (sr);
        SynthParams p;
        p.chipMode = 0; p.psgMix = 0.9f; p.psgWave = 1;
        p.op[0].level = 0; p.op[1].level = 0; p.op[2].level = 0; p.op[3].level = 0; // PSG only!
        eng.noteOn (60, 1.0f, p);
        std::vector<float> L (N), R (N);
        for (int pos = 0; pos < N; pos += 128)
            eng.render (L.data() + pos, R.data() + pos, std::min (128, N - pos), p, 120.0);
        const double rms = rmsOf (L, 2000);
        std::snprintf (msg, sizeof (msg), "PSG sub-octave rms=%.3f", rms);
        check (rms > 0.08 && allFinite (L), "SN76489-style PSG sub-chip audible & clean", msg);
    }

    // 5. All 32 Handcrafted Factory Presets render audibly with zero NaN/Inf!
    {
        int count = 0;
        const ArcadePreset* presets = factoryPresets (count);
        int audible = 0, finite = 0;
        std::vector<float> L (16384), R (16384);
        for (int i = 0; i < count; ++i)
        {
            OpArcadeEngine eng;
            eng.prepare (sr);
            eng.noteOn (60, 0.9f, presets[i].p);
            for (int pos = 0; pos < 16384; pos += 128)
                eng.render (L.data() + pos, R.data() + pos, std::min (128, 16384 - pos), presets[i].p, 120.0);
            if (allFinite (L) && allFinite (R)) ++finite;
            if (rmsOf (L, 256) > 0.015 || rmsOf (R, 256) > 0.015) ++audible;
        }
        std::snprintf (msg, sizeof (msg), "%d/%d presets audible, %d/%d finite", audible, count, finite, count);
        check (audible == count && finite == count, "All 32 factory presets audible & finite", msg);
    }

    // 6. Per-Voice Hybrid ZDF-SVF LPF, Pre-HPF & Extreme Resonance Stability
    {
        int cnt = 0;
        SynthParams pByp = factoryPresets (cnt)[0].p;
        pByp.chorusMix = 0.0f; pByp.delayMix = 0.0f;

        SynthParams pLp = pByp;
        pLp.filtMode = 1; pLp.filtCut = 0.25f; pLp.filtRes = 0.35f;

        SynthParams pExt = pByp;
        pExt.filtMode = 1; pExt.filtCut = 0.45f; pExt.filtRes = 1.0f; pExt.filtDrive = 1.0f;

        auto hfSlew = [&] (const SynthParams& sp, int note, double& outPeak) {
            OpArcadeEngine eng; eng.prepare (sr);
            eng.noteOn (note, 1.0f, sp);
            std::vector<float> L (N), R (N);
            for (int pos = 0; pos < N; pos += 128)
                eng.render (L.data() + pos, R.data() + pos, std::min (128, N - pos), sp, 120.0);
            double slew = 0.0; outPeak = 0.0;
            for (int i = 2001; i < N; ++i)
            {
                slew += std::fabs (L[(size_t) i] - L[(size_t) (i - 1)]);
                outPeak = std::max (outPeak, (double) std::fabs (L[(size_t) i]));
            }
            return slew / (double) (N - 2001) / (rmsOf (L, 2000) + 1e-6);
        };

        auto bassRms = [&] (const SynthParams& sp) {
            OpArcadeEngine eng; eng.prepare (sr);
            eng.noteOn (36, 1.0f, sp); // C2 = ~65.4 Hz
            std::vector<float> L (N), R (N);
            for (int pos = 0; pos < N; pos += 128)
                eng.render (L.data() + pos, R.data() + pos, std::min (128, N - pos), sp, 120.0);
            return rmsOf (L, 2000);
        };

        double pkByp = 0.0, pkLp = 0.0, pkExt = 0.0;
        const double slByp = hfSlew (pByp, 60, pkByp);
        const double slLp  = hfSlew (pLp,  60, pkLp);
        hfSlew (pExt, 60, pkExt);

        SynthParams pHpf = pByp; pHpf.filtHpf = 0.72f;
        const double rLowByp = bassRms (pByp);
        const double rLowHpf = bassRms (pHpf);

        std::snprintf (msg, sizeof (msg), "LP24 slew %.3f->%.3f, HPF C2 %.3f->%.3f, extPk=%.3f",
                       slByp, slLp, rLowByp, rLowHpf, pkExt);
        check (slLp < slByp * 0.45 && rLowHpf < rLowByp * 0.45 && pkExt < 1.5 && pkExt > 0.05,
               "Per-voice LP24 + Pre-HPF + Res=1.0 stability", msg);
    }

    // 7. Per-Voice Polyphonic Filter ADSR Envelope Brightness Decay
    {
        int cnt = 0;
        SynthParams pEnv = factoryPresets (cnt)[0].p;
        pEnv.chorusMix = 0.0f; pEnv.delayMix = 0.0f;
        pEnv.filtMode = 1; pEnv.filtCut = 0.15f; pEnv.filtRes = 0.30f;
        pEnv.filtEnvAmt = 0.75f; pEnv.filtAtt = 0.0f; pEnv.filtDec = 0.18f; pEnv.filtSus = 0.0f;

        OpArcadeEngine eng; eng.prepare (sr);
        eng.noteOn (48, 1.0f, pEnv);
        std::vector<float> L (N), R (N);
        for (int pos = 0; pos < N; pos += 128)
            eng.render (L.data() + pos, R.data() + pos, std::min (128, N - pos), pEnv, 120.0);

        auto windowSlew = [&] (int a, int b) {
            double slew = 0.0, sq = 0.0;
            for (int i = a + 1; i < b; ++i)
            {
                slew += std::fabs (L[(size_t) i] - L[(size_t) (i - 1)]);
                sq   += (double) L[(size_t) i] * L[(size_t) i];
            }
            const double r = std::sqrt (sq / std::max (1, b - a - 1));
            return (slew / std::max (1, b - a - 1)) / (r + 1e-6);
        };
        const double slEarly = windowSlew (200, 1800);
        const double slLate  = windowSlew (15000, 21000);
        std::snprintf (msg, sizeof (msg), "envSlew early=%.3f -> late=%.3f", slEarly, slLate);
        check (slEarly > slLate * 1.3 && allFinite (L),
               "Per-voice Filter ADSR sweeps cutoff down", msg);
    }

    std::printf ("\nSummary: %d passed, %d failed\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
