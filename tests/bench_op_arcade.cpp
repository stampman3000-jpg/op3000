// OP-ARCADE — Real-Time CPU Benchmark (44.1 kHz, 128-frame = 2902.5 us budget).
#include "../bridge/OpArcadePresets.h"
#include "../dsp/OpArcadeEngine.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

using namespace oparcade;

static void runBench (const char* name, int chipMode, bool psg, bool chorus, bool delay, bool vcf, int notes)
{
    const double sr = 44100.0;
    const int blk = 128;
    int cnt = 0;
    SynthParams p = factoryPresets (cnt)[0].p;
    p.chipMode = chipMode;
    p.psgMix = psg ? 0.4f : 0.0f;
    p.chorusMix = chorus ? 0.4f : 0.0f;
    p.delayMix = delay ? 0.3f : 0.0f;
    if (vcf)
    {
        p.filtMode = 1; p.filtCut = 0.55f; p.filtRes = 0.45f; p.filtHpf = 0.20f;
        p.filtDrive = 0.25f; p.filtEnvAmt = 0.50f; p.filtLfo = 0.20f;
    }

    OpArcadeEngine eng;
    eng.prepare (sr);
    const int chord[6] = { 48, 52, 55, 60, 64, 67 };
    for (int i = 0; i < notes; ++i) eng.noteOn (chord[i % 6], 1.0f, p);

    std::vector<float> L (blk), R (blk);
    for (int b = 0; b < 40; ++b) eng.render (L.data(), R.data(), blk, p, 120.0);

    std::vector<double> us;
    us.reserve (400);
    for (int b = 0; b < 400; ++b)
    {
        const auto t0 = std::chrono::steady_clock::now();
        eng.render (L.data(), R.data(), blk, p, 120.0);
        const auto t1 = std::chrono::steady_clock::now();
        us.push_back (std::chrono::duration<double, std::micro> (t1 - t0).count());
    }
    double sum = 0.0;
    for (double x : us) sum += x;
    std::sort (us.begin(), us.end());
    const double avg = sum / (double) us.size();
    const double p99 = us[(size_t) (us.size() * 99 / 100)];
    const double budget = (128.0 / sr) * 1e6;
    std::printf ("%-38s %8.1f %8.1f %7.2f%% %7.2f%%\n",
                 name, avg, p99, (avg / budget) * 100.0, (p99 / budget) * 100.0);
}

int main()
{
    std::printf ("=== OP-ARCADE Dual-Chip CPU Benchmark (44.1 kHz, 128-frame = 2902.5 us) ===\n");
    std::printf ("%-38s %8s %8s %8s %8s\n", "Scenario", "Avg(us)", "p99(us)", "Avg%128", "p99%128");
    std::printf ("----------------------------------------------------------------------------\n");
    runBench ("YM2151 OPM Only (6 voices, dry)",    0, false, false, false, false, 6);
    runBench ("YM2151 OPM + Chorus + Delay",        0, false, true,  true,  false, 6);
    runBench ("YM2612 OPN2 Only (6 voices, dry)",   1, false, false, false, false, 6);
    runBench ("YM2612 OPN2 + Chorus + Delay",       1, false, true,  true,  false, 6);
    runBench ("HYBRID Dual-Chip (dry)",             2, false, false, false, false, 6);
    runBench ("HYBRID Dual-Chip + PSG + FX",        2, true,  true,  true,  false, 6);
    runBench ("HYBRID + PSG + VCF LP24+HPF+FX",     2, true,  true,  true,  true,  6);
    return 0;
}
