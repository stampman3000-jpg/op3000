// OP-ARCADE — Dual-Chip Silicon-Accurate YM2151 (OPM) + YM2612 (OPN2) + PSG Hybrid MPC Instrument.
// Built on the official Akai MPC Plugin Builder SDK (mpc_plugin.h).

#include "../../../bridge/OpArcadePresets.h"
#include "../../../dsp/OpArcadeEngine.h"
#include "mpc_plugin.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace oparcade {

enum ParamIdx : int
{
    P_VOLUME = 0, // Harmless master level when MPC nudges param 0 on load

    // Page 1: ARCADE (1..16)
    P_CHIP_MODE = 1,
    P_ALG,
    P_FEEDBACK,
    P_VOICE_MODE,
    P_FM_BITE,
    P_MACRO_ATT,
    P_MACRO_DEC,
    P_MACRO_REL,
    P_DETUNE,
    P_LFO_SPEED,
    P_LFO_PM,
    P_LFO_AM,
    P_PSG_MIX,
    P_PCB_TONE,
    P_CHORUS_MIX,
    P_DELAY_MIX,

    // Page 2: OPS (17..48 = 4 ops * 8 params: MUL, DT1, DT2, LVL, AR, D1R, SL, RR)
    P_OP_BASE = 17,
    // 49..52: OP1..OP4 AM Enable (saved in presets & automatable)
    P_OP_AM_BASE = 49,

    // Page 3: PCB / FX (53..67)
    P_CHIP_BLEND = 53,
    P_OPN2_OCT,
    P_OPN2_DET,
    P_OPM_NOISE,
    P_SSG_EG,
    P_DAC_GRIT,
    P_PSG_WAVE,
    P_PSG_DECAY,
    P_VEL_SENS,
    P_PORTA,
    P_BEND_RANGE,
    P_LFO_WAVE,
    P_DELAY_DIV,
    P_DELAY_FB,
    P_DELAY_TONE,

    // Non-automatable UI Telemetry (68..71)
    P_METER_L = 68,
    P_METER_R = 69,
    P_STAT_INFO = 70,
    P_STAT_CHIP = 71,

    // Page 4: FILTER (72..84 automatable, 85 non-automatable UI status)
    P_FILT_MODE = 72,
    P_FILT_CUT,
    P_FILT_RES,
    P_FILT_HPF,
    P_FILT_DRIVE,
    P_FILT_KEY,
    P_FILT_ENV_AMT,
    P_FILT_VEL,
    P_FILT_ATT,
    P_FILT_DEC,
    P_FILT_SUS,
    P_FILT_REL,
    P_FILT_LFO,
    P_STAT_FILT,

    P_COUNT = 86
};

class OpArcadePlugin final : public mpc::Plugin
{
public:
    OpArcadePlugin() : mpc::Plugin (makeInfo()), engine_ (std::make_unique<OpArcadeEngine>())
    {
        registerAllParams();
        registerFactoryPresets();

        for (auto& d : uiDirty_) d.store (false, std::memory_order_relaxed);
        setProgram (0);
        worker_ = std::thread ([this] { uiWorkerLoop(); });
    }

    ~OpArcadePlugin() override
    {
        stopWorker_.store (true, std::memory_order_release);
        if (worker_.joinable()) worker_.join();
    }

    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo info;
        info.name         = "OP3000";
        info.vendor       = "Stampman3000";
        info.uniqueId     = MPC_FOURCC ('O', 'P', 'A', 'R');
        info.version      = 1000;
        info.isInstrument = true;
        info.numInputs    = 0;
        info.numOutputs   = 2;
        info.wantsMidi    = true;
        info.sendsMidi    = false;
        info.latencySamples = 0;
        info.tailSamples  = 44100 * 3;
        return info;
    }

    void prepare (double sr, int) override
    {
        engine_->prepare (sr > 8000.0 ? sr : 44100.0);
        readyForHost_.store (true, std::memory_order_release);
    }

    void reset() override
    {
        engine_->allNotesOff();
        engine_->resetChips();
    }

    void parameterChanged (int index, float) override
    {
        if (index == P_CHIP_MODE || index == P_ALG || index == P_VOICE_MODE || index == P_PCB_TONE)
        {
            bumpStatus (P_STAT_INFO);
            bumpStatus (P_STAT_CHIP);
        }
        if (index == P_FILT_MODE || index == P_FILT_CUT || index == P_FILT_RES || index == P_FILT_HPF)
            bumpStatus (P_STAT_FILT);
    }

    bool formatValue (int index, float plainValue, std::string& out) override
    {
        char buf[64];
        if (index == P_STAT_INFO)
        {
            const char* pname = programName (currentProgram());
            static const char* kChips[3] = { "OPM", "OPN2", "HYB" };
            const int cm = clampi (getInt (P_CHIP_MODE), 0, 2);
            const int av = activeVoicesAtomic_.load (std::memory_order_relaxed);
            std::snprintf (buf, sizeof (buf), "%.17s | %s | V:%d", pname ? pname : "OP3000", kChips[cm], av);
            out = buf;
            return true;
        }
        if (index == P_STAT_CHIP)
        {
            const int cm = clampi (getInt (P_CHIP_MODE), 0, 2);
            const int alg = clampi (getInt (P_ALG), 0, 7);
            if (cm == 0)      std::snprintf (buf, sizeof (buf), "YM2151 ARCADE • ALG %d", alg);
            else if (cm == 1) std::snprintf (buf, sizeof (buf), "YM2612 MEGA • ALG %d", alg);
            else              std::snprintf (buf, sizeof (buf), "YM2151 + YM2612 • ALG %d", alg);
            out = buf;
            return true;
        }
        if (index == P_STAT_FILT)
        {
            static const char* kFModes[4] = { "LP12", "LP24", "BP12", "HP12" };
            const int   fm  = clampi (getInt (P_FILT_MODE), 0, 3);
            const float fc  = 25.0f * std::exp2f (clampf (get (P_FILT_CUT) * 0.01f, 0.0f, 1.0f) * 9.57f);
            const int   res = (int) std::lround (get (P_FILT_RES));
            const float hpf = get (P_FILT_HPF);
            char fcStr[24];
            if (fc >= 1000.0f) std::snprintf (fcStr, sizeof (fcStr), "%.1fkHz", (double) (fc * 0.001f));
            else               std::snprintf (fcStr, sizeof (fcStr), "%.0fHz", (double) fc);
            if (res >= 1)
                std::snprintf (buf, sizeof (buf), "%s • %s • R:%d%%", kFModes[fm], fcStr, res);
            else if (hpf > 0.5f)
            {
                const float hc = 20.0f * std::exp2f (clampf (hpf * 0.01f, 0.0f, 1.0f) * 7.8138f);
                if (hc >= 1000.0f) std::snprintf (buf, sizeof (buf), "%s • %s • HP:%.1fk", kFModes[fm], fcStr, (double) (hc * 0.001f));
                else               std::snprintf (buf, sizeof (buf), "%s • %s • HP:%.0f", kFModes[fm], fcStr, (double) hc);
            }
            else
                std::snprintf (buf, sizeof (buf), "%s • %s • HPF OFF", kFModes[fm], fcStr);
            out = buf;
            return true;
        }
        if (index == P_FILT_CUT)
        {
            const float fc = 25.0f * std::exp2f (clampf (plainValue * 0.01f, 0.0f, 1.0f) * 9.57f);
            if (fc >= 1000.0f) std::snprintf (buf, sizeof (buf), "%.1f kHz", (double) (fc * 0.001f));
            else               std::snprintf (buf, sizeof (buf), "%.0f Hz", (double) fc);
            out = buf; return true;
        }
        if (index == P_FILT_HPF)
        {
            if (plainValue <= 0.5f) { out = "Off"; return true; }
            const float fc = 20.0f * std::exp2f (clampf (plainValue * 0.01f, 0.0f, 1.0f) * 7.8138f);
            if (fc >= 1000.0f) std::snprintf (buf, sizeof (buf), "%.1f kHz", (double) (fc * 0.001f));
            else               std::snprintf (buf, sizeof (buf), "%.0f Hz", (double) fc);
            out = buf; return true;
        }
        if (index == P_FILT_ATT)
        {
            const float t = 0.001f * std::pow (3500.0f, clampf (plainValue * 0.01f, 0.0f, 1.0f));
            if (t >= 1.0f) std::snprintf (buf, sizeof (buf), "%.2f s", (double) t);
            else           std::snprintf (buf, sizeof (buf), "%.0f ms", (double) (t * 1000.0f));
            out = buf; return true;
        }
        if (index == P_FILT_DEC || index == P_FILT_REL)
        {
            const float t = 0.008f * std::pow (750.0f, clampf (plainValue * 0.01f, 0.0f, 1.0f));
            if (t >= 1.0f) std::snprintf (buf, sizeof (buf), "%.2f s", (double) t);
            else           std::snprintf (buf, sizeof (buf), "%.0f ms", (double) (t * 1000.0f));
            out = buf; return true;
        }
        if (index == P_FM_BITE || index == P_MACRO_ATT || index == P_MACRO_DEC ||
            index == P_MACRO_REL || index == P_OPN2_DET || index == P_FILT_ENV_AMT)
        {
            std::snprintf (buf, sizeof (buf), "%+.0f%%", (double) plainValue);
            out = buf; return true;
        }
        if (index == P_CHIP_BLEND)
        {
            const int opmPct = clampi (100 - (int) std::lround (plainValue), 0, 100);
            const int opnPct = 100 - opmPct;
            std::snprintf (buf, sizeof (buf), "OPM %d / OPN %d", opmPct, opnPct);
            out = buf; return true;
        }
        if (index == P_OPM_NOISE)
        {
            const int v = (int) std::lround (plainValue);
            if (v <= 0) out = "Off";
            else { std::snprintf (buf, sizeof (buf), "Ch7:%d", v); out = buf; }
            return true;
        }
        return false;
    }

    void process (mpc::ProcessContext& ctx) override
    {
        const int n = ctx.numFrames;
        if (n <= 0 || ! ctx.outputs || ctx.numOutputs < 1 || ! ctx.outputs[0]) return;
        float* outL = ctx.outputs[0];
        float* outR = (ctx.numOutputs >= 2 && ctx.outputs[1]) ? ctx.outputs[1] : outL;

        const SynthParams p = snapshotParams();
        const double bpm = (ctx.transport.valid && ctx.transport.tempoValid && ctx.transport.bpm > 20.0)
            ? ctx.transport.bpm : 120.0;

        int cursor = 0;
        for (int e = 0; e < ctx.numMidiEvents; ++e)
        {
            const auto& ev = ctx.midi[e];
            const int target = clampi (ev.sampleOffset, cursor, n);
            if (target > cursor)
            {
                engine_->render (outL + cursor, outR + cursor, target - cursor, p, bpm);
                cursor = target;
            }
            handleMidi (ev, p);
        }
        if (cursor < n)
            engine_->render (outL + cursor, outR + cursor, n - cursor, p, bpm);

        // Publish stereo LED meter & voice count telemetry
        const int mL = std::min (7, (int) (std::sqrt (clampf (engine_->peakL() * 1.15f, 0.0f, 1.0f)) * 7.6f));
        const int mR = std::min (7, (int) (std::sqrt (clampf (engine_->peakR() * 1.15f, 0.0f, 1.0f)) * 7.6f));
        meterLAtomic_.store (mL, std::memory_order_relaxed);
        meterRAtomic_.store (mR, std::memory_order_relaxed);
        activeVoicesAtomic_.store (engine_->activeVoices(), std::memory_order_relaxed);
    }

private:
    void handleMidi (const mpc::MidiEvent& ev, const SynthParams& p)
    {
        if (ev.isNoteOn())
        {
            engine_->noteOn (ev.note(), (float) ev.velocity() / 127.0f, p);
        }
        else if (ev.isNoteOff())
        {
            engine_->noteOff (ev.note());
        }
        else if (ev.isPitchBend())
        {
            const int raw = (int) (ev.data[1] & 0x7F) | ((int) (ev.data[2] & 0x7F) << 7);
            engine_->setPitchBend ((float) (raw - 8192) / 8192.0f);
        }
        else if (ev.isController())
        {
            const int cc = ev.controller();
            const int val = ev.data[2] & 0x7F;
            if (cc == 1)        engine_->setModWheel ((float) val / 127.0f);
            else if (cc == 64)  engine_->setSustain (val >= 64);
            else if (cc == 120 || cc == 123) engine_->allNotesOff();
        }
    }

    SynthParams snapshotParams() const
    {
        SynthParams p;
        p.volume     = get (P_VOLUME) * 0.01f;
        p.chipMode   = getInt (P_CHIP_MODE);
        p.alg        = getInt (P_ALG);
        p.feedback   = getInt (P_FEEDBACK);
        p.voiceMode  = getInt (P_VOICE_MODE);
        p.fmBite     = get (P_FM_BITE) * 0.01f;
        p.macroAtt   = get (P_MACRO_ATT) * 0.01f;
        p.macroDec   = get (P_MACRO_DEC) * 0.01f;
        p.macroRel   = get (P_MACRO_REL) * 0.01f;
        p.detune     = get (P_DETUNE) * 0.01f;
        p.lfoSpeed   = get (P_LFO_SPEED) * 0.01f;
        p.lfoPm      = get (P_LFO_PM) * 0.01f;
        p.lfoAm      = get (P_LFO_AM) * 0.01f;
        p.psgMix     = get (P_PSG_MIX) * 0.01f;
        p.pcbTone    = getInt (P_PCB_TONE);
        p.chorusMix  = get (P_CHORUS_MIX) * 0.01f;
        p.delayMix   = get (P_DELAY_MIX) * 0.01f;

        for (int i = 0; i < 4; ++i)
        {
            const int b = P_OP_BASE + i * 8;
            p.op[i].mul   = getInt (b + 0);
            p.op[i].dt1   = getInt (b + 1) - 3;
            p.op[i].dt2   = getInt (b + 2);
            p.op[i].level = getInt (b + 3);
            p.op[i].ar    = getInt (b + 4);
            p.op[i].d1r   = getInt (b + 5);
            p.op[i].d2r   = (p.op[i].d1r >= 10) ? (p.op[i].d1r / 4) : 0;
            p.op[i].sl    = getInt (b + 6);
            p.op[i].rr    = getInt (b + 7);
            p.op[i].am    = getInt (P_OP_AM_BASE + i);
        }

        p.chipBlend  = get (P_CHIP_BLEND) * 0.01f;
        p.opn2Oct    = getInt (P_OPN2_OCT) - 1;
        p.opn2Detune = get (P_OPN2_DET) * 0.01f;
        p.opmNoise   = getInt (P_OPM_NOISE);
        p.ssgEg      = getInt (P_SSG_EG);
        p.dacGrit    = get (P_DAC_GRIT) * 0.01f;
        p.psgWave    = getInt (P_PSG_WAVE);
        p.psgDecay   = get (P_PSG_DECAY) * 0.01f;
        p.velSens    = get (P_VEL_SENS) * 0.01f;
        p.porta      = get (P_PORTA) * 0.01f;
        p.bendRange  = getInt (P_BEND_RANGE);
        p.lfoWave    = getInt (P_LFO_WAVE);
        p.delayDiv   = getInt (P_DELAY_DIV);
        p.delayFb    = get (P_DELAY_FB) * 0.01f;
        p.delayTone  = get (P_DELAY_TONE) * 0.01f;

        p.filtMode   = getInt (P_FILT_MODE);
        p.filtCut    = get (P_FILT_CUT) * 0.01f;
        p.filtRes    = get (P_FILT_RES) * 0.01f;
        p.filtHpf    = get (P_FILT_HPF) * 0.01f;
        p.filtDrive  = get (P_FILT_DRIVE) * 0.01f;
        p.filtKey    = get (P_FILT_KEY) * 0.01f;
        p.filtEnvAmt = get (P_FILT_ENV_AMT) * 0.01f;
        p.filtVel    = get (P_FILT_VEL) * 0.01f;
        p.filtAtt    = get (P_FILT_ATT) * 0.01f;
        p.filtDec    = get (P_FILT_DEC) * 0.01f;
        p.filtSus    = get (P_FILT_SUS) * 0.01f;
        p.filtRel    = get (P_FILT_REL) * 0.01f;
        p.filtLfo    = get (P_FILT_LFO) * 0.01f;
        return p;
    }

    void registerAllParams()
    {
        using namespace mpc;
        auto addNonAuto = [&] (Param p) { p.automatable = false; addParam (p); };

        // 0: Master Volume
        addParam (floatParam  ("volume",     "Volume",      0.0f, 100.0f, 80.0f, "%", 0));

        // 1..16: Page 1 ARCADE
        addParam (choiceParam ("chip_mode",  "Chip Mode",   { "OPM 2151", "OPN2 2612", "HYBRID" }, 2));
        addParam (choiceParam ("alg",        "Algorithm",   { "ALG 0", "ALG 1", "ALG 2", "ALG 3", "ALG 4", "ALG 5", "ALG 6", "ALG 7" }, 4));
        addParam (choiceParam ("feedback",   "Feedback",    { "0", "1", "2", "3", "4", "5", "6", "7" }, 6));
        addParam (choiceParam ("voice_mode", "Voice Mode",  { "Poly", "Mono", "Unison 2" }, 0));
        addParam (floatParam  ("fm_bite",    "FM Bite",     -100.0f, 100.0f, 0.0f, "%", 0));
        addParam (floatParam  ("macro_att",  "Attack Mac",  -100.0f, 100.0f, 0.0f, "%", 0));
        addParam (floatParam  ("macro_dec",  "Decay Mac",   -100.0f, 100.0f, 0.0f, "%", 0));
        addParam (floatParam  ("macro_rel",  "Release Mac", -100.0f, 100.0f, 0.0f, "%", 0));
        addParam (floatParam  ("detune",     "Detune",      0.0f, 100.0f, 30.0f, "%", 0));
        addParam (floatParam  ("lfo_speed",  "LFO Speed",   0.0f, 100.0f, 40.0f, "%", 0));
        addParam (floatParam  ("lfo_pm",     "Vib / PM",    0.0f, 100.0f, 12.0f, "%", 0));
        addParam (floatParam  ("lfo_am",     "Trem / AM",   0.0f, 100.0f, 0.0f,  "%", 0));
        addParam (floatParam  ("psg_mix",    "PSG Sub",     0.0f, 100.0f, 0.0f,  "%", 0));
        addParam (choiceParam ("pcb_tone",   "PCB Tone",    { "Crisp", "MD1 Warm", "Lo-Fi" }, 0));
        addParam (floatParam  ("chorus_mix", "Chorus",      0.0f, 100.0f, 38.0f, "%", 0));
        addParam (floatParam  ("delay_mix",  "Delay Mix",   0.0f, 100.0f, 22.0f, "%", 0));

        // 17..48: Page 2 OPS (4 ops * 8 params)
        const std::vector<std::string> mulNames = { "0.5", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15" };
        const std::vector<std::string> dt1Names = { "-3", "-2", "-1", "0", "+1", "+2", "+3" };
        const std::vector<std::string> dt2Names = { "1.00", "1.41", "1.57", "1.73" };

        for (int i = 0; i < 4; ++i)
        {
            const std::string pfx = "op" + std::to_string (i + 1) + "_";
            const std::string nfx = "OP" + std::to_string (i + 1) + " ";
            addParam (choiceParam ((pfx + "mul").c_str(), (nfx + "Ratio").c_str(),   mulNames, 1));
            addParam (choiceParam ((pfx + "dt1").c_str(), (nfx + "Detune").c_str(),  dt1Names, 3));
            addParam (choiceParam ((pfx + "dt2").c_str(), (nfx + "Coarse").c_str(),  dt2Names, 0));
            addParam (intParam    ((pfx + "lvl").c_str(), (nfx + "Level").c_str(),   0, 100, (i == 3 ? 92 : 72), "%"));
            addParam (intParam    ((pfx + "ar").c_str(),  (nfx + "Attack").c_str(),  0, 31,  26));
            addParam (intParam    ((pfx + "d1r").c_str(), (nfx + "Decay").c_str(),   0, 31,  8));
            addParam (intParam    ((pfx + "sl").c_str(),  (nfx + "Sustain").c_str(), 0, 15,  3));
            addParam (intParam    ((pfx + "rr").c_str(),  (nfx + "Release").c_str(), 0, 15,  7));
        }

        // 49..52: OP1..4 AM Enable
        for (int i = 0; i < 4; ++i)
        {
            const std::string id = "op" + std::to_string (i + 1) + "_am";
            const std::string nm = "OP" + std::to_string (i + 1) + " AM";
            addParam (boolParam (id.c_str(), nm.c_str(), false));
        }

        // 53..67: Page 3 PCB / FX
        addParam (floatParam  ("chip_blend", "Chip Blend", 0.0f, 100.0f, 50.0f, "%", 0));
        addParam (choiceParam ("opn2_oct",   "OPN2 Oct",   { "-1", "0", "+1" }, 1));
        addParam (floatParam  ("opn2_det",   "OPN2 Det",   -100.0f, 100.0f, 22.0f, "%", 0));
        addParam (intParam    ("opm_noise",  "OPM Noise",  0, 31, 0));
        addParam (choiceParam ("ssg_eg",     "SSG-EG",     { "Off", "Loop", "Alt", "Inv" }, 0));
        addParam (floatParam  ("dac_grit",   "DAC Grit",   0.0f, 100.0f, 0.0f, "%", 0));
        addParam (choiceParam ("psg_wave",   "PSG Wave",   { "Square", "Sub -1", "Sub -2", "Pulse", "Noise" }, 1));
        addParam (floatParam  ("psg_decay",  "PSG Decay",  0.0f, 100.0f, 35.0f, "%", 0));
        addParam (floatParam  ("vel_sens",   "Vel Sens",   0.0f, 100.0f, 55.0f, "%", 0));
        addParam (floatParam  ("porta",      "Glide",      0.0f, 100.0f, 0.0f,  "%", 0));
        addParam (choiceParam ("bend_range", "Bend Rng",   { "0","1","2","3","4","5","6","7","8","9","10","11","12" }, 2));
        addParam (choiceParam ("lfo_wave",   "LFO Wave",   { "Saw", "Square", "Tri", "Noise" }, 2));
        addParam (choiceParam ("delay_div",  "Delay Time", { "1/16", "1/8T", "1/8", "1/8D", "1/4", "1/2" }, 3));
        addParam (floatParam  ("delay_fb",   "Delay Fbk",  0.0f, 90.0f,  35.0f, "%", 0));
        addParam (floatParam  ("delay_tone", "Delay Tone", 0.0f, 100.0f, 65.0f, "%", 0));

        // 68..71: Non-automatable UI Telemetry
        addNonAuto (intParam ("meter_l",   "Meter L", 0, 7, 0));
        addNonAuto (intParam ("meter_r",   "Meter R", 0, 7, 0));
        addNonAuto (intParam ("stat_info", "Info",    0, 999, 0));
        addNonAuto (intParam ("stat_chip", "Chip",    0, 999, 0));

        // 72..85: Page 4 Per-Voice Hybrid Filter
        addParam (choiceParam ("filt_mode",    "Filter Mode", { "LP12", "LP24", "BP12", "HP12" }, 1));
        addParam (floatParam  ("filt_cut",     "Cutoff",      0.0f, 100.0f, 100.0f, "%", 0));
        addParam (floatParam  ("filt_res",     "Resonance",   0.0f, 100.0f, 0.0f,   "%", 0));
        addParam (floatParam  ("filt_hpf",     "HPF Cut",     0.0f, 100.0f, 0.0f,   "%", 0));
        addParam (floatParam  ("filt_drive",   "OTA Drive",   0.0f, 100.0f, 0.0f,   "%", 0));
        addParam (floatParam  ("filt_key",     "Keytrack",    0.0f, 100.0f, 50.0f,  "%", 0));
        addParam (floatParam  ("filt_env_amt", "Env Depth",  -100.0f, 100.0f, 0.0f, "%", 0));
        addParam (floatParam  ("filt_vel",     "Filt Vel",    0.0f, 100.0f, 40.0f,  "%", 0));
        addParam (floatParam  ("filt_att",     "Filt Atk",    0.0f, 100.0f, 0.0f,   "%", 0));
        addParam (floatParam  ("filt_dec",     "Filt Dec",    0.0f, 100.0f, 38.0f,  "%", 0));
        addParam (floatParam  ("filt_sus",     "Filt Sus",    0.0f, 100.0f, 35.0f,  "%", 0));
        addParam (floatParam  ("filt_rel",     "Filt Rel",    0.0f, 100.0f, 35.0f,  "%", 0));
        addParam (floatParam  ("filt_lfo",     "LFO -> Cut",  0.0f, 100.0f, 0.0f,   "%", 0));
        addNonAuto (intParam  ("stat_filt",    "Filt Stat",   0, 999, 0));
    }

    void registerFactoryPresets()
    {
        int count = 0;
        const ArcadePreset* list = factoryPresets (count);
        for (int i = 0; i < count; ++i)
        {
            const SynthParams& p = list[i].p;
            std::vector<float> vals (P_COUNT, 0.0f);
            vals[P_VOLUME]     = p.volume * 100.0f;
            vals[P_CHIP_MODE]  = (float) p.chipMode;
            vals[P_ALG]        = (float) p.alg;
            vals[P_FEEDBACK]   = (float) p.feedback;
            vals[P_VOICE_MODE] = (float) p.voiceMode;
            vals[P_FM_BITE]    = p.fmBite * 100.0f;
            vals[P_MACRO_ATT]  = p.macroAtt * 100.0f;
            vals[P_MACRO_DEC]  = p.macroDec * 100.0f;
            vals[P_MACRO_REL]  = p.macroRel * 100.0f;
            vals[P_DETUNE]     = p.detune * 100.0f;
            vals[P_LFO_SPEED]  = p.lfoSpeed * 100.0f;
            vals[P_LFO_PM]     = p.lfoPm * 100.0f;
            vals[P_LFO_AM]     = p.lfoAm * 100.0f;
            vals[P_PSG_MIX]    = p.psgMix * 100.0f;
            vals[P_PCB_TONE]   = (float) p.pcbTone;
            vals[P_CHORUS_MIX] = p.chorusMix * 100.0f;
            vals[P_DELAY_MIX]  = p.delayMix * 100.0f;

            for (int op = 0; op < 4; ++op)
            {
                const int b = P_OP_BASE + op * 8;
                vals[b + 0] = (float) p.op[op].mul;
                vals[b + 1] = (float) (p.op[op].dt1 + 3);
                vals[b + 2] = (float) p.op[op].dt2;
                vals[b + 3] = (float) p.op[op].level;
                vals[b + 4] = (float) p.op[op].ar;
                vals[b + 5] = (float) p.op[op].d1r;
                vals[b + 6] = (float) p.op[op].sl;
                vals[b + 7] = (float) p.op[op].rr;
                vals[P_OP_AM_BASE + op] = (float) p.op[op].am;
            }

            vals[P_CHIP_BLEND] = p.chipBlend * 100.0f;
            vals[P_OPN2_OCT]   = (float) (p.opn2Oct + 1);
            vals[P_OPN2_DET]   = p.opn2Detune * 100.0f;
            vals[P_OPM_NOISE]  = (float) p.opmNoise;
            vals[P_SSG_EG]     = (float) p.ssgEg;
            vals[P_DAC_GRIT]   = p.dacGrit * 100.0f;
            vals[P_PSG_WAVE]   = (float) p.psgWave;
            vals[P_PSG_DECAY]  = p.psgDecay * 100.0f;
            vals[P_VEL_SENS]   = p.velSens * 100.0f;
            vals[P_PORTA]      = p.porta * 100.0f;
            vals[P_BEND_RANGE] = (float) p.bendRange;
            vals[P_LFO_WAVE]   = (float) p.lfoWave;
            vals[P_DELAY_DIV]  = (float) p.delayDiv;
            vals[P_DELAY_FB]   = p.delayFb * 100.0f;
            vals[P_DELAY_TONE] = p.delayTone * 100.0f;

            vals[P_FILT_MODE]    = (float) p.filtMode;
            vals[P_FILT_CUT]     = p.filtCut * 100.0f;
            vals[P_FILT_RES]     = p.filtRes * 100.0f;
            vals[P_FILT_HPF]     = p.filtHpf * 100.0f;
            vals[P_FILT_DRIVE]   = p.filtDrive * 100.0f;
            vals[P_FILT_KEY]     = p.filtKey * 100.0f;
            vals[P_FILT_ENV_AMT] = p.filtEnvAmt * 100.0f;
            vals[P_FILT_VEL]     = p.filtVel * 100.0f;
            vals[P_FILT_ATT]     = p.filtAtt * 100.0f;
            vals[P_FILT_DEC]     = p.filtDec * 100.0f;
            vals[P_FILT_SUS]     = p.filtSus * 100.0f;
            vals[P_FILT_REL]     = p.filtRel * 100.0f;
            vals[P_FILT_LFO]     = p.filtLfo * 100.0f;

            addProgram (list[i].name, std::move (vals));
        }
    }

    void storeParamQuiet (int idx, float plain)
    {
        if (idx < 0 || idx >= numParams()) return;
        values_[(size_t) idx]->store (params_[(size_t) idx].toNormalised (plain), std::memory_order_relaxed);
    }

    void bumpStatus (int idx)
    {
        const int next = (getInt (idx) + 1) % 999;
        storeParamQuiet (idx, (float) next);
        uiDirty_[(size_t) idx].store (true, std::memory_order_release);
    }

    void uiWorkerLoop()
    {
        int prevML = -1, prevMR = -1, prevV = -1, prevProg = -1;
        int tick = 0;
        while (! stopWorker_.load (std::memory_order_acquire))
        {
            std::this_thread::sleep_for (std::chrono::milliseconds (5));
            if (stopWorker_.load (std::memory_order_acquire)) break;
            if (++tick < 6) continue;
            tick = 0;
            if (! readyForHost_.load (std::memory_order_acquire)) continue;

            const int prog = currentProgram();
            if (prog != prevProg)
            {
                prevProg = prog;
                bumpStatus (P_STAT_INFO);
                bumpStatus (P_STAT_CHIP);
                bumpStatus (P_STAT_FILT);
            }
            const int mL = meterLAtomic_.load (std::memory_order_relaxed);
            if (mL != prevML)
            {
                prevML = mL;
                storeParamQuiet (P_METER_L, (float) mL);
                uiDirty_[P_METER_L].store (true, std::memory_order_release);
            }
            const int mR = meterRAtomic_.load (std::memory_order_relaxed);
            if (mR != prevMR)
            {
                prevMR = mR;
                storeParamQuiet (P_METER_R, (float) mR);
                uiDirty_[P_METER_R].store (true, std::memory_order_release);
            }
            const int av = activeVoicesAtomic_.load (std::memory_order_relaxed);
            if (av != prevV)
            {
                prevV = av;
                bumpStatus (P_STAT_INFO);
            }

            for (int i = 0; i < P_COUNT; ++i)
            {
                if (uiDirty_[(size_t) i].exchange (false, std::memory_order_acq_rel))
                    notifyHostParameterChanged (i);
            }
        }
    }

    std::unique_ptr<OpArcadeEngine> engine_;
    std::atomic<bool> readyForHost_ { false };
    std::atomic<bool> stopWorker_ { false };
    std::atomic<int>  meterLAtomic_ { 0 };
    std::atomic<int>  meterRAtomic_ { 0 };
    std::atomic<int>  activeVoicesAtomic_ { 0 };
    std::array<std::atomic<bool>, P_COUNT> uiDirty_ {};
    std::thread worker_;
};

} // namespace oparcade

using MpcOpArcadePlugin = oparcade::OpArcadePlugin;
MPC_PLUGIN_ENTRY (MpcOpArcadePlugin)
