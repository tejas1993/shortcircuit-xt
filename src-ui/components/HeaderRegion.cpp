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

#include "HeaderRegion.h"
#include "SCXTEditor.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/ToggleButtonRadioGroup.h"
#include "sst/jucegui/data/Discrete.h"
#include "widgets/ShortCircuitMenuButton.h"

namespace scxt::ui
{

namespace cmsg = scxt::messaging::client;

// TODO: Move this ot a proper vu meter in jucegui
struct VUMeter : juce::Component
{
    HeaderRegion *header{nullptr};
    VUMeter(HeaderRegion *hr) : header(hr) {}
    void paint(juce::Graphics &g) override
    {
        if (!header)
            return;

        g.setColour(juce::Colours::black);
        g.fillAll();

        g.setColour(juce::Colour(0xFF, 0x90, 0x00));
        auto lBox = getLocalBounds()
                        .withHeight(getHeight() * 0.5)
                        .withWidth(getWidth() * header->vuLevel[0])
                        .reduced(0, 1);
        auto rBox = getLocalBounds()
                        .withTrimmedTop(getHeight() * 0.5)
                        .withWidth(getWidth() * header->vuLevel[1])
                        .reduced(0, 1);
        g.fillRect(lBox);
        g.fillRect(rBox);
    }
};

struct spData : sst::jucegui::data::Discrete
{
    HeaderRegion *headerRegion{nullptr};
    spData(HeaderRegion *hr) : headerRegion(hr) {}
    std::string getLabel() const override { return "Page"; }
    int getValue() const override
    {
        if (headerRegion && headerRegion->editor)
            return headerRegion->editor->activeScreen;
        return 0;
    }

    void setValueFromGUI(const int &f) override
    {
        if (headerRegion && headerRegion->editor)
        {
            headerRegion->editor->setActiveScreen((SCXTEditor::ActiveScreen)f);
            headerRegion->repaint();
        }
    }

    void setValueFromModel(const int &f) override {}

    std::string getValueAsStringFor(int i) const override
    {
        if (headerRegion && headerRegion->editor)
        {
            switch ((SCXTEditor::ActiveScreen)i)
            {
            case SCXTEditor::MULTI:
                return "MULTI";
            case SCXTEditor::MIXER:
                return "MIXER";
            case SCXTEditor::PLAY:
                return "PLAY";
            }
        }
        assert(false);
        return "-error-";
    }

    int getMax() const override { return 2; }
};

HeaderRegion::HeaderRegion(SCXTEditor *e) : HasEditor(e)
{
    selectedPage = std::make_unique<sst::jucegui::components::ToggleButtonRadioGroup>();
    selectedPageData = std::make_unique<spData>(this);
    selectedPage->setSource(selectedPageData.get());

    addAndMakeVisible(*selectedPage);

    vuMeter = std::make_unique<VUMeter>(this);
    addAndMakeVisible(*vuMeter);

    scMenu = std::make_unique<widgets::ShortCircuitMenuButton>();
    scMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->editor->showMainMenu();
    });
    addAndMakeVisible(*scMenu);
}

HeaderRegion::~HeaderRegion()
{
    if (selectedPage)
        selectedPage->setSource(nullptr);
}

void HeaderRegion::resized()
{
    selectedPage->setBounds(4, 4, 170, getHeight() - 8);
    scMenu->setBounds(getWidth() - getHeight() - 8, 4, getHeight() - 8, getHeight() - 8);
    vuMeter->setBounds(getWidth() - getHeight() - 8 - 140, 4, 135, getHeight() - 8);
}

void HeaderRegion::setVULevel(float L, float R)
{
    if (vuMeter)
    {
        float ub = 1.4f;
        vuLevel[0] = sqrt(std::clamp(L, 0.f, ub)) / sqrt(ub);
        vuLevel[1] = sqrt(std::clamp(R, 0.f, ub)) / sqrt(ub);

        vuMeter->repaint();
    }
}

} // namespace scxt::ui