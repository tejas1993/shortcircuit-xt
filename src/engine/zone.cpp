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

#include "zone.h"
#include "bus.h"
#include "group.h"
#include "modulation/modulators/steplfo.h"
#include "part.h"
#include "engine.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "group_and_zone_impl.h"

namespace scxt::engine
{
void Zone::process(Engine &e)
{
    namespace blk = sst::basic_blocks::mechanics;
    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    std::array<voice::Voice *, maxVoices> toCleanUp;
    size_t cleanupIdx{0};
    gatedVoiceCount = 0;
    for (auto &v : voiceWeakPointers)
    {
        if (v && v->isVoiceAssigned)
        {
            if (v->process())
            {
                if (outputInfo.routeTo == DEFAULT_BUS)
                {
                    blk::accumulate_from_to<blockSize>(v->output[0], output[0]);
                    blk::accumulate_from_to<blockSize>(v->output[1], output[1]);
                }
                else if (outputInfo.routeTo >= 0)
                {
                    auto &bs = getEngine()->getPatch()->busses;
                    blk::accumulate_from_to<blockSize>(
                        v->output[0], bs.busByAddress(outputInfo.routeTo).output[0]);
                    blk::accumulate_from_to<blockSize>(
                        v->output[1], bs.busByAddress(outputInfo.routeTo).output[1]);
                }
            }
            if (!v->isVoicePlaying)
            {
                toCleanUp[cleanupIdx++] = v;
            }

            if (v->isGated)
            {
                gatedVoiceCount++;
            }
        }
    }
    for (int i = 0; i < cleanupIdx; ++i)
    {

#if DEBUG_VOICE_LIFECYCLE
        SCLOG("Cleanup Voice at " << SCDBGV((int)toCleanUp[i]->key));
#endif
        toCleanUp[i]->cleanupVoice();
    }
}

void Zone::addVoice(voice::Voice *v)
{
    if (activeVoices == 0)
    {
        parentGroup->addActiveZone();
    }
    activeVoices++;
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == nullptr)
        {
            nv = v;
            return;
        }
    }
    assert(false);
}
void Zone::removeVoice(voice::Voice *v)
{
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == v)
        {
            nv = nullptr;
            activeVoices--;
            if (activeVoices == 0)
            {
                parentGroup->removeActiveZone();
            }
            return;
        }
    }
    assert(false);
}

engine::Engine *Zone::getEngine()
{
    if (parentGroup && parentGroup->parentPart && parentGroup->parentPart->parentPatch)
        return parentGroup->parentPart->parentPatch->parentEngine;
    return nullptr;
}

void Zone::initialize()
{
    for (auto &v : voiceWeakPointers)
        v = nullptr;

    for (auto &l : lfoStorage)
    {
        modulation::modulators::clear_lfo(l);
    }
}

void Zone::setupOnUnstream(const engine::Engine &e)
{
    sampleLoadOverridesMapping = false;
    attachToSample(*(e.getSampleManager()));
    for (int p = 0; p < processorCount; ++p)
    {
        setupProcessorControlDescriptions(p, processorStorage[p].type);
    }
}

bool Zone::attachToSample(const sample::SampleManager &manager, int index)
{
    auto &s = sampleData[index];
    if (s.sampleID.isValid())
    {
        samplePointers[index] = manager.getSample(s.sampleID);
    }
    else
    {
        samplePointers[index].reset();
    }

    if (sampleLoadOverridesMapping)
    {
        if (samplePointers[index] && index == 0)
        {
            const auto &m = samplePointers[index]->meta;

            if (m.rootkey_present)
                mapping.rootKey = m.key_root;
            if (m.key_present)
            {
                mapping.keyboardRange = {m.key_low, m.key_high};
            }
            if (m.vel_present)
            {
                mapping.velocityRange = {m.vel_low, m.vel_high};
            }
        }

        if (samplePointers[index])
        {
            const auto &m = samplePointers[index]->meta;
            s.startSample = 0;
            s.endSample = samplePointers[index]->getSampleLength();
            if (m.loop_present)
            {
                s.startLoop = m.loop_start;
                s.endLoop = m.loop_end;
                s.loopActive = true;
            }
            else
            {
                s.startLoop = 0;
                s.endLoop = s.endSample;
            }
        }
    }
    return samplePointers[index] != nullptr;
}

std::string Zone::toStringPlayMode(const PlayMode &p)
{
    switch (p)
    {
    case NORMAL:
        return "normal";
    case ONE_SHOT:
        return "oneshot";
    case ON_RELEASE:
        return "onrelease";
    }
    return "normal";
}

Zone::PlayMode Zone::fromStringPlayMode(const std::string &s)
{
    static auto inverse = makeEnumInverse<Zone::PlayMode, Zone::toStringPlayMode>(
        Zone::PlayMode::NORMAL, Zone::PlayMode::ON_RELEASE);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return NORMAL;
    return p->second;
}

std::string Zone::toStringLoopMode(const LoopMode &p)
{
    switch (p)
    {
    case LOOP_DURING_VOICE:
        return "during_voice";
    case LOOP_WHILE_GATED:
        return "while_gated";
    case LOOP_FOR_COUNT:
        return "for_count";
    }
    return "during_voice";
}

Zone::LoopMode Zone::fromStringLoopMode(const std::string &s)
{
    static auto inverse = makeEnumInverse<Zone::LoopMode, Zone::toStringLoopMode>(
        Zone::LoopMode::LOOP_DURING_VOICE, Zone::LoopMode::LOOP_FOR_COUNT);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return LOOP_DURING_VOICE;
    return p->second;
}

std::string Zone::toStringLoopDirection(const LoopDirection &p)
{
    switch (p)
    {
    case FORWARD_ONLY:
        return "forward";
    case ALTERNATE_DIRECTIONS:
        return "alternate";
    }
    return "forward";
}

Zone::LoopDirection Zone::fromStringLoopDirection(const std::string &s)
{
    static auto inverse = makeEnumInverse<Zone::LoopDirection, Zone::toStringLoopDirection>(
        Zone::LoopDirection::FORWARD_ONLY, Zone::LoopDirection::ALTERNATE_DIRECTIONS);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return FORWARD_ONLY;
    return p->second;
}

template struct HasGroupZoneProcessors<Zone>;
} // namespace scxt::engine