/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_MODULATION_MODULATORS_STEPLFO_H
#define SCXT_SRC_MODULATION_MODULATORS_STEPLFO_H

#include "datamodel/timedata.h"
#include "utils.h"
#include <array>
#include <vector>
#include <utility>
#include "infrastructure/rng_gen.h"

namespace scxt::modulation::modulators
{
static constexpr int stepLfoSteps{32};

struct StepLFOStorage
{
    StepLFOStorage() { std::fill(data.begin(), data.end(), 0.f); }
    std::array<float, stepLfoSteps> data;
    int repeat{16};
    float rate{0.f};
    float smooth{0.f};
    float shuffle{0.f};
    bool temposync{false};

    // These enum values are streamed. Do not change them.
    enum TriggerModes
    {
        VOICE = 0,
        FREERUN = 1,
        RANDOM = 2,
        RELEASE = 3
    } triggermode{VOICE};

    bool cyclemode{true};
    bool onlyonce{false};
    bool isStep{true};
    // add midi sync capabilities

    bool operator==(const StepLFOStorage &other) const
    {
        return data == other.data && repeat == other.repeat && rate == other.rate &&
               smooth == other.smooth && shuffle == other.shuffle && temposync == other.temposync &&
               triggermode == other.triggermode && cyclemode == other.cyclemode &&
               onlyonce == other.onlyonce;
    }
    bool operator!=(const StepLFOStorage &other) const { return !(*this == other); }
};

enum LFOPresets
{
    lp_custom = 0,
    lp_clear,
    lp_sine,
    lp_tri,
    lp_square,
    lp_ramp_up,
    lp_ramp_down,
    lp_noise,
    lp_noise_mean3,
    lp_noise_mean5,
    lp_tremolo_tri,
    lp_tremolo_sin,
    n_lfopresets,
};

inline std::string getLfoPresetName(LFOPresets p)
{
    switch (p)
    {
    case lp_custom:
        return "custom";
    case lp_clear:
        return "clear";
    case lp_sine:
        return "sine";
    case lp_tri:
        return "tri";
    case lp_square:
        return "square";
    case lp_ramp_up:
        return "ramp_up";
    case lp_ramp_down:
        return "ramp_down";
    case lp_noise:
        return "noise";
    case lp_noise_mean3:
        return "noise_mean3";
    case lp_noise_mean5:
        return "noise_mean5";
    case lp_tremolo_tri:
        return "tremolo_tri";
    case lp_tremolo_sin:
        return "tremolo_sin";
    case n_lfopresets:
        throw std::logic_error("Called with n_lfopresets");
        return "ERROR";
    }
    throw std::logic_error("Called with non-switchable preset");
}

void clear_lfo(StepLFOStorage &settings);
void load_lfo_preset(LFOPresets preset, StepLFOStorage &settings, infrastructure::RNGGen &);
float lfo_ipol(float *step_history, float phase, float smooth, int odd);

struct StepLFO : MoveableOnly<StepLFO>, SampleRateSupport
{
  public:
    StepLFO();
    ~StepLFO();
    void assign(StepLFOStorage *settings, const float *rate, datamodel::TimeData *td,
                infrastructure::RNGGen &);
    void sync();
    void process(int samples);
    float output;

    void UpdatePhaseIncrement();

  protected:
    long state;
    long state_tminus1;
    float phase, phaseInc;
    float wf_history[4];
    float ratemult;
    int shuffle_id;
    const float *rate;
    datamodel::TimeData *td;
    StepLFOStorage *settings;
};
} // namespace scxt::modulation::modulators

#endif // SCXT_SRC_MODULATION_MODULATORS_STEPLFO_H
