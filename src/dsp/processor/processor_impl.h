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

#ifndef SCXT_SRC_DSP_PROCESSOR_PROCESSOR_IMPL_H
#define SCXT_SRC_DSP_PROCESSOR_PROCESSOR_IMPL_H

#include "configuration.h"
#include "processor.h"
#include "engine/memory_pool.h"

namespace scxt::dsp::processor
{
struct SCXTVFXConfig
{
    using BaseClass = Processor;
    static constexpr int blockSize{scxt::blockSize};

    static void preReservePool(BaseClass *b, size_t s) { b->memoryPool->preReservePool(s); }

    static uint8_t *checkoutBlock(BaseClass *b, size_t s)
    {
        return b->memoryPool->checkoutBlock(s);
    }

    static void returnBlock(BaseClass *b, uint8_t *d, size_t s)
    {
        b->memoryPool->returnBlock(d, s);
    }

    static void setFloatParam(BaseClass *b, size_t index, float val) { b->param[index] = val; }
    static float getFloatParam(BaseClass *b, size_t index) { return b->param[index]; }

    static float dbToLinear(BaseClass *b, float db) { return dsp::dbTable.dbToLinear(db); }
    static float getSampleRate(BaseClass *b) { return b->getSampleRate(); }
    static float getSampleRateInv(BaseClass *b) { return b->getSampleRateInv(); }
    static float equalNoteToPitch(BaseClass *b, float f)
    {
        return tuning::equalTuning.note_to_pitch(f);
    }
};

#define HAS_MEMFN(M)                                                                               \
    template <typename T> class HasMemFn_##M                                                       \
    {                                                                                              \
        using No = uint8_t;                                                                        \
        using Yes = uint64_t;                                                                      \
        static_assert(sizeof(No) != sizeof(Yes));                                                  \
        template <typename C> static Yes test(decltype(&C::M) *);                                  \
        template <typename C> static No test(...);                                                 \
                                                                                                   \
      public:                                                                                      \
        enum                                                                                       \
        {                                                                                          \
            value = sizeof(test<T>(nullptr)) == sizeof(Yes)                                        \
        };                                                                                         \
    };
HAS_MEMFN(initVoiceEffect);
HAS_MEMFN(initVoiceEffectParams);
HAS_MEMFN(processStereo);
HAS_MEMFN(processMonoToMono);
HAS_MEMFN(processMonoToStereo);
#undef HAS_MEMFN

template <typename T> struct SSTVoiceEffectShim : T
{
    void init() override
    {
        if constexpr (HasMemFn_initVoiceEffect<T>::value)
        {
            this->initVoiceEffect();
        }
    }

    void init_params() override
    {
        if constexpr (HasMemFn_initVoiceEffectParams<T>::value)
        {
            this->initVoiceEffectParams();
        }
    }

    virtual bool canProcessMono() override
    {
        return HasMemFn_processMonoToMono<T>::value || HasMemFn_processMonoToStereo<T>::value;
    }
    virtual bool monoInputCreatesStereoOutput() override
    {
        return HasMemFn_processMonoToStereo<T>::value;
    }

    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override
    {
        this->processStereo(datainL, datainR, dataoutL, dataoutR, pitch);
        if constexpr (HasMemFn_processStereo<T>::value)
        {
            this->processStereo(datainL, datainR, dataoutL, dataoutR, pitch);
        }
        else if constexpr (HasMemFn_processMonoToMono<T>::value)
        {
            this->processMonoToMono(datainL, dataoutL, pitch);
            this->processMonoToMono(datainR, dataoutR, pitch);
        }
        else
        {
            assert(false);
        }
    }

    virtual void process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch) override
    {
        if constexpr (HasMemFn_processMonoToMono<T>::value)
        {
            this->processMonoToMono(datain, dataoutL, pitch);
        }
        else if constexpr (HasMemFn_processMonoToStereo<T>::value)
        {
            this->processMonoToMono(datain, dataoutL, dataoutR, pitch);
        }
    }
};

template <typename T>
void Processor::setupProcessor(T *that, ProcessorType t, engine::MemoryPool *mp, float *fp, int *ip)
{
    myType = t;
    memoryPool = mp;
    param = fp;
    iparam = ip;

    parameter_count = T::numFloatParams;
    for (int i = 0; i < parameter_count; ++i)
        this->ctrlmode_desc[i] = that->paramAt(i);
}

} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_PROCESSOR_IMPL_H
