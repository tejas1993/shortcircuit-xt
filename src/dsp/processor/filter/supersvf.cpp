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

#include "supersvf.h"
#include "configuration.h"
#include "tuning/equal.h"

namespace scxt::dsp::processor::filter
{

// TODO: A bit more of a comprehensive review of this code
static constexpr float INV_2BLOCK_SIZE{1.f / float(BLOCK_SIZE << 1)};
const auto INV_2BLOCK_SIZE_128{_mm_set1_ps(INV_2BLOCK_SIZE)};

enum
{
    ssvf_LP = 0,
    ssvf_BP,
    ssvf_HP,
};

//-------------------------------------------------------------------------------------------------------

SuperSVF::SuperSVF(engine::MemoryPool *mp, float *fp, int *ip)
    : Processor(ProcessorType::proct_SuperSVF, mp, fp, ip), mPolyphase(2, true)
{
    parameter_count = 2;
    ctrlmode_desc[0] = datamodel::pitchInOctavesFromA440().withName("cutoff");
    ctrlmode_desc[1] = datamodel::pmd().asPercent().withName("resonance");
    assert(iparam);

    suspend();
}

//-------------------------------------------------------------------------------------------------------

void SuperSVF::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 0.f;
    param[1] = 0.0f;
    iparam[0] = 0;
    iparam[1] = 0;
}

//-------------------------------------------------------------------------------------------------------

// TODO: Isn't this must _mm_set1_ps
inline __m128 SplatVector(float x)
{
    __m128 v = _mm_load_ss(&x);
    return _mm_shuffle_ps(v, v, 0);
}

void SuperSVF::calc_coeffs()
{
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]) || (lastiparam[1] != iparam[1]))
    {
        float f = 440.f * tuning::equalTuning.note_to_pitch(param[0] * 12.f);
        float F1 = 2.0 * sin(M_PI * std::min(0.11, f * (0.25 * samplerate_inv))); // 4x oversampling

        float Reso = sqrt(std::clamp(param[1], 0.f, 1.f));

        float overshoot = (iparam[1] == 1) ? 0.05 : 0.075;
        float Q1 = 2.0 - Reso * (2.0 + overshoot) + F1 * F1 * overshoot * 0.9;

        Q1 = std::min({Q1, 2.00f, 2.00f - 1.52f * F1});

        float NewClipDamp = 0.1 * Reso * F1;

        const float a = 0.65;

        float NewGain = 1 - a * Reso;

        // Set interpolators
        __m128 nFreq = SplatVector(F1);
        dFreq = _mm_sub_ps(nFreq, Freq);
        dFreq = _mm_mul_ps(dFreq, INV_2BLOCK_SIZE_128);

        __m128 nQ = SplatVector(Q1);
        dQ = _mm_sub_ps(nQ, Q);
        dQ = _mm_mul_ps(dQ, INV_2BLOCK_SIZE_128);

        __m128 nClipDamp = SplatVector(NewClipDamp);
        dClipDamp = _mm_sub_ps(nClipDamp, ClipDamp);
        dClipDamp = _mm_mul_ps(dClipDamp, INV_2BLOCK_SIZE_128);

        __m128 nGain = SplatVector(NewGain);
        dGain = _mm_sub_ps(nGain, Gain);
        dGain = _mm_mul_ps(dGain, INV_2BLOCK_SIZE_128);

        // Update paramtriggers
        lastparam[0] = param[0];
        lastparam[1] = param[1];
        lastiparam[0] = iparam[0];
    }
    else
    {
        dFreq = _mm_setzero_ps();
        dQ = _mm_setzero_ps();
        dClipDamp = _mm_setzero_ps();
        dGain = _mm_setzero_ps();
    }
}

//-------------------------------------------------------------------------------------------------------

template <bool Stereo, bool FourPole>
void SuperSVF::ProcessT(float *DataInL, float *DataInR, float *DataOutL, float *DataOutR,
                        float Pitch)
{
    calc_coeffs();

    assert(iparam[0] < 3);
    assert(iparam[0] >= 0);
    assert(iparam[1] >= 0);
    assert(iparam[1] < 2);

    const int bs2 = BLOCK_SIZE << 1;
    float PolyphaseInL alignas(16)[bs2];
    float PolyphaseInR alignas(16)[bs2];

    for (int k = 0; k < BLOCK_SIZE; k++)
    {
        __m128 Input;
        if (Stereo)
        {
            Input = _mm_unpacklo_ps(_mm_load_ss(&DataInL[k]), _mm_load_ss(&DataInR[k]));
        }
        else
        {
            Input = _mm_load_ss(&DataInL[k]);
        }

        __m128 MiddleOutput;

        if (FourPole)
        {
            Input = _mm_movelh_ps(Input, LastOutput);
            MiddleOutput = process_internal(Input, iparam[0]);
            Input = _mm_movelh_ps(Input, MiddleOutput);
            LastOutput = process_internal(Input, iparam[0]);
        }
        else
        {
            MiddleOutput = process_internal(Input, iparam[0]);
            LastOutput = process_internal(Input, iparam[0]);
        }

        if (Stereo)
        {
            if (FourPole)
            {
                _mm_store_ss(&PolyphaseInL[(k << 1)], _mm_movehl_ps(MiddleOutput, MiddleOutput));
                _mm_store_ss(&PolyphaseInR[(k << 1)],
                             _mm_shuffle_ps(MiddleOutput, MiddleOutput, _MM_SHUFFLE(3, 3, 3, 3)));
                _mm_store_ss(&PolyphaseInL[(k << 1) + 1], _mm_movehl_ps(LastOutput, LastOutput));
                _mm_store_ss(&PolyphaseInR[(k << 1) + 1],
                             _mm_shuffle_ps(LastOutput, LastOutput, _MM_SHUFFLE(3, 3, 3, 3)));
            }
            else
            {
                _mm_store_ss(&PolyphaseInL[(k << 1)], MiddleOutput);
                _mm_store_ss(&PolyphaseInR[(k << 1)],
                             _mm_shuffle_ps(MiddleOutput, MiddleOutput, _MM_SHUFFLE(1, 1, 1, 1)));
                _mm_store_ss(&PolyphaseInL[(k << 1) + 1], LastOutput);
                _mm_store_ss(&PolyphaseInR[(k << 1) + 1],
                             _mm_shuffle_ps(LastOutput, LastOutput, _MM_SHUFFLE(1, 1, 1, 1)));
            }
        }
        else
        {
            if (FourPole)
            {
                _mm_store_ss(&PolyphaseInL[(k << 1)], _mm_movehl_ps(MiddleOutput, MiddleOutput));
                _mm_store_ss(&PolyphaseInL[(k << 1) + 1], _mm_movehl_ps(LastOutput, LastOutput));
            }
            else
            {
                _mm_store_ss(&PolyphaseInL[(k << 1)], MiddleOutput);
                _mm_store_ss(&PolyphaseInL[(k << 1) + 1], LastOutput);
            }
        }
    }

    // Polyphase filter
    // if mono, use valid buffers but ignore the result
    mPolyphase.process_block_D2(PolyphaseInL, Stereo ? PolyphaseInR : PolyphaseInL, bs2, DataOutL,
                                Stereo ? DataOutR : PolyphaseInR);
}

//-------------------------------------------------------------------------------------------------------

inline __m128 SuperSVF::process_internal(__m128 x, int Mode)
{
    Freq = _mm_add_ps(Freq, dFreq);
    Q = _mm_add_ps(Q, dQ);
    ClipDamp = _mm_add_ps(ClipDamp, dClipDamp);

    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);

    __m128 L = _mm_add_ps(Reg[1], _mm_mul_ps(Freq, Reg[0]));
    __m128 H = _mm_sub_ps(_mm_sub_ps(x, L), _mm_mul_ps(Q, Reg[0]));
    __m128 B = _mm_add_ps(Reg[0], _mm_mul_ps(Freq, H));

    __m128 L2 = _mm_add_ps(L, _mm_mul_ps(Freq, B));
    __m128 H2 = _mm_sub_ps(_mm_sub_ps(x, L2), _mm_mul_ps(Q, B));
    __m128 B2 = _mm_add_ps(B, _mm_mul_ps(Freq, H2));

    __m128 Out[3];
    Out[0] = L2;
    Out[1] = B2;
    Out[2] = H2;

    Reg[0] = _mm_mul_ps(B2, Reg[2]);
    Reg[1] = _mm_mul_ps(L2, Reg[2]);
    Reg[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(ClipDamp, _mm_mul_ps(B2, B2))));

    Gain = _mm_add_ps(Gain, dGain); // Gain

    // mode switch needed
    return _mm_mul_ps(Out[Mode], Gain);
}

//-------------------------------------------------------------------------------------------------------

void SuperSVF::process_stereo(float *DataInL, float *DataInR, float *DataOutL, float *DataOutR,
                              float Pitch)
{
    if (iparam[1] > 0)
    {
        ProcessT<true, true>(DataInL, DataInR, DataOutL, DataOutR, Pitch);
    }
    else
    {
        ProcessT<true, false>(DataInL, DataInR, DataOutL, DataOutR, Pitch);
    }
}

void SuperSVF::process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch)
{
    if (iparam[1] > 0)
    {
        ProcessT<false, true>(datain, datain, dataoutL, dataoutL, pitch);
    }
    else
    {
        ProcessT<false, false>(datain, datain, dataoutL, dataoutL, pitch);
    }
}

//-------------------------------------------------------------------------------------------------------

void SuperSVF::suspend()
{
    Reg[0] = _mm_setzero_ps();
    Reg[1] = _mm_setzero_ps();
    Reg[2] = _mm_setzero_ps();

    Freq = _mm_setzero_ps();
    Q = _mm_setzero_ps();
    ClipDamp = _mm_setzero_ps();
    Gain = _mm_setzero_ps();

    LastOutput = _mm_setzero_ps();

    lastiparam[1] = -1; // force coeff recalculation
}

//-------------------------------------------------------------------------------------------------------

size_t SuperSVF::getIntParameterCount() { return 2; }

//-------------------------------------------------------------------------------------------------------

const char *SuperSVF::getIntParameterLabel(int ip_id)
{
    if (ip_id == 0)
        return ("mode");
    else if (ip_id == 1)
        return ("order");
    return ("");
}

//-------------------------------------------------------------------------------------------------------

size_t SuperSVF::getIntParameterChoicesCount(int ip_id)
{
    if (ip_id == 0)
        return 3;
    return 2;
}

//-------------------------------------------------------------------------------------------------------

const char *SuperSVF::getIntParameterChoicesLabel(int ip_id, int c_id)
{
    if (ip_id == 0)
    {
        switch (c_id)
        {
        case ssvf_LP:
            return ("LP");
        case ssvf_BP:
            return ("BP");
        case ssvf_HP:
            return ("HP");
        }
    }
    else if (ip_id == 1)
    {
        switch (c_id)
        {
        case 0:
            return ("2-pole");
        case 1:
            return ("4-pole");
        }
    }
    return ("");
}

} // namespace scxt::dsp::processor::filter
