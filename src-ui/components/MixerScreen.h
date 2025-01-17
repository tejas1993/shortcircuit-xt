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

#ifndef SCXT_SRC_UI_COMPONENTS_MIXERSCREEN_H
#define SCXT_SRC_UI_COMPONENTS_MIXERSCREEN_H

#include "configuration.h"
#include "HasEditor.h"
#include "SCXTEditor.h"
#include "browser/BrowserPane.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui
{
namespace browser
{
struct BrowserPane;
}
namespace mixer
{
struct PartEffectsPane;
struct BusPane;
} // namespace mixer
struct MixerScreen : juce::Component, HasEditor
{
    static constexpr int sideWidths = 196; // copied from multi for now

    MixerScreen(SCXTEditor *e);
    ~MixerScreen();
    void visibilityChanged() override;
    void resized() override;

    void selectBus(int index);

    void onBusEffectFullData(
        int bus, int slot,
        const std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams> &,
        const engine::BusEffectStorage &);

    std::unique_ptr<browser::BrowserPane> browser;
    std::array<std::unique_ptr<mixer::PartEffectsPane>, engine::Bus::maxEffectsPerBus> partPanes;
    std::unique_ptr<mixer::BusPane> busPane;

    using partFXMD_t = std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams>;
    using partFXStorage_t = std::pair<partFXMD_t, engine::BusEffectStorage>;
    using busPartsFX_t = std::array<partFXStorage_t, engine::Bus::maxEffectsPerBus>;
    using busFX_t = std::array<busPartsFX_t, scxt::maxOutputs>;

    busFX_t busEffectsData;

    using busSend_t = std::array<engine::Bus::BusSendStorage, scxt::maxBusses>;
    busSend_t busSendData;

    void onBusSendData(int bus, const engine::Bus::BusSendStorage &s);

    // The effects have names like 'flanger' and 'delay' internally but we
    // want alternate display names here.
    std::string effectDisplayName(engine::AvailableBusEffects, bool forMenu);

    void setFXSlotToType(int bus, int slot, engine::AvailableBusEffects t);
    void showFXSelectionMenu(int bus, int slot);
    void sendBusSendStorage(int bus);

    void setVULevelForBusses(
        const std::array<std::array<std::atomic<float>, 2>, engine::Patch::Busses::busCount> &x);
};
} // namespace scxt::ui
#endif // SHORTCIRCUIT_SENDFXSCREEN_H
