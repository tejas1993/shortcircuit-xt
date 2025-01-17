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

#ifndef SCXT_SRC_UI_COMPONENTS_BROWSER_BROWSERPANE_H
#define SCXT_SRC_UI_COMPONENTS_BROWSER_BROWSERPANE_H

#include <vector>
#include "filesystem/import.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/NamedPanel.h>
#include "components/HasEditor.h"

#include "sst/jucegui/components/ToggleButtonRadioGroup.h"

namespace scxt::ui::browser
{
struct DevicesPane;
struct FavoritesPane;
struct SearchPane;
struct BrowserPane : public HasEditor, sst::jucegui::components::NamedPanel
{
    std::unique_ptr<sst::jucegui::components::ToggleButtonRadioGroup> selectedFunction;
    std::unique_ptr<sst::jucegui::data::Discrete> selectedFunctionData;

    BrowserPane(SCXTEditor *e);
    ~BrowserPane();
    void resized() override;

    void resetRoots();
    std::vector<std::pair<fs::path, std::string>> roots;

    std::unique_ptr<DevicesPane> devicesPane;
    std::unique_ptr<FavoritesPane> favoritesPane;
    std::unique_ptr<SearchPane> searchPane;

    void selectPane(int);
    int selectedPane{0};
};
} // namespace scxt::ui::browser

#endif // SHORTCIRCUITXT_BROWSERPANE_H
