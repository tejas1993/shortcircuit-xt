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
#ifndef SCXT_SRC_ENGINE_PATCH_H
#define SCXT_SRC_ENGINE_PATCH_H

#include "utils.h"
#include "part.h"
#include <memory>
#include <array>
#include <sst/cpputils.h>

namespace scxt::engine
{
struct Engine;
struct Patch : MoveableOnly<Patch>, SampleRateSupport
{
    Patch() : id(PatchID::next()) { reset(); }

    // TODO - does this really belong in the patch? I think it probably does
    struct Busses
    {
        Busses() : mainBus(MAIN_0)
        {
            std::fill(partToVSTRouting.begin(), partToVSTRouting.end(), 0);
            std::fill(auxToVSTRouting.begin(), auxToVSTRouting.end(), 0);
            int adr = PART_0;
            for (auto &p : partBusses)
            {
                p.address = (BusAddress)adr;
                p.busSendStorage.supportsSends = true;
                adr++;
            }
            adr = AUX_0;
            for (auto &p : auxBusses)
            {
                p.address = (BusAddress)adr;
                adr++;
            }
        }

        inline void clear()
        {
            // TODO - be more parsimonious - only clear things in use
            mainBus.clear();
            for (auto &b : partBusses)
                b.clear();
            for (auto &b : auxBusses)
                b.clear();
        }

        static constexpr int busCount{numParts + numAux + 1};

        Bus mainBus;

        std::array<Bus, numParts> partBusses;
        // here '0' means main bus
        std::array<int16_t, numParts> partToVSTRouting{};

        std::array<Bus, numAux> auxBusses;
        std::array<int16_t, numAux> auxToVSTRouting{};

        Bus &busByAddress(engine::BusAddress b)
        {
            if (b == MAIN_0)
                return mainBus;
            else if (b >= PART_0 && b < AUX_0)
                return partBusses[b - PART_0];
            else if (b < AUX_0 + numAux)
                return auxBusses[b - AUX_0];

            return mainBus;
        }

        void sendInitialBusInfo(const Engine &e)
        {
            mainBus.sendAllBusEffectInfoToClient(e);
            mainBus.sendBusSendStorageToClient(e);
            for (auto &p : partBusses)
            {
                p.sendAllBusEffectInfoToClient(e);
                p.sendBusSendStorageToClient(e);
            }
            for (auto &p : auxBusses)
            {
                p.sendAllBusEffectInfoToClient(e);
                p.sendBusSendStorageToClient(e);
            }
        }
    } busses;

    void process(Engine &e);

    void reset()
    {
        // If it is the year 2112 and you just had a regtest fail because
        // this is earlier than the streaming version you just changed, then
        // this software lived too long
        streamingVersion = 0x21120101;
        for (int i = 0; i < numParts; ++i)
        {
            parts[i] = std::make_unique<Part>(i);
            parts[i]->parentPatch = this;
        }
    }

    void setupBussesOnUnstream(Engine &e);
    PatchID id;
    uint64_t streamingVersion{0}; // we use hex dates for these

    Engine *parentEngine{nullptr};

    /**
     * Get a part by the index in array
     */
    const std::unique_ptr<Part> &getPart(int i) const
    {
        assert(i >= 0 && i < numParts);
        return parts[i];
    }

    /**
     * Get the index of a part by the PartID.
     */
    int getPartIndex(const PartID &pid) const
    {
        for (const auto &[idx, r] : sst::cpputils::enumerate(parts))
            if (r->id == pid)
                return idx;
        return -1;
    }

    /**
     * Get the part by part ID. May throw.
     */
    const std::unique_ptr<Part> &getPart(const PartID &pid) const
    {
        auto idx = getPartIndex(pid);
        if (idx < 0 || idx >= parts.size())
            throw SCXTError("Unable to locate part " + pid.to_string() + " in patch " +
                            id.to_string());
        return parts[idx];
    }

    void onSampleRateChanged() override;
    typedef std::array<std::unique_ptr<Part>, numParts> partContainer_t;

    partContainer_t::iterator begin() noexcept { return parts.begin(); }
    partContainer_t::const_iterator cbegin() const noexcept { return parts.cbegin(); }

    partContainer_t::iterator end() noexcept { return parts.end(); }
    partContainer_t::const_iterator cend() const noexcept { return parts.cend(); }

    const partContainer_t &getParts() const { return parts; }

  private:
    partContainer_t parts;
};
} // namespace scxt::engine

#endif