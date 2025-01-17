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

#ifndef SCXT_SRC_MESSAGING_CLIENT_GROUP_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_GROUP_MESSAGES_H

#include "client_macros.h"
#include "engine/group.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{

using groupOutputInfoUpdate_t = std::pair<bool, engine::Group::GroupOutputInfo>;
SERIAL_TO_CLIENT(GroupOutputInfoUpdated, s2c_update_group_output_info, groupOutputInfoUpdate_t,
                 onGroupOutputInfoUpdated);

inline void updateGroupOutputInfo(const engine::Group::GroupOutputInfo &payload,
                                  const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Group State
    auto sz = engine.getSelectionManager()->currentlySelectedGroups();
    if (!sz.empty())
    {
        cont.scheduleAudioThreadCallback([zs = sz, info = payload](auto &eng) {
            for (const auto &[p, g, z] : zs)
            {
                eng.getPatch()->getPart(p)->getGroup(g)->outputInfo = info;
            }
        });
    }
}
CLIENT_TO_SERIAL(UpdateGroupOutputInfo, c2s_update_group_output_info,
                 engine::Group::GroupOutputInfo, updateGroupOutputInfo(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_GROUP_MESSAGES_H
