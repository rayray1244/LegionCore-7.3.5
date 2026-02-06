/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LEGION_HARDCORE_CHALLENGE_H
#define LEGION_HARDCORE_CHALLENGE_H

class Player;

namespace HardcoreChallenge
{
    constexpr int32 ChoiceIdMain = 910000;
    constexpr int32 ChoiceIdDeadlyTrial = 910001;

    constexpr int32 ResponseOpenDeadlyTrial = 1;
    constexpr int32 ResponseEnable = 1;
    constexpr int32 ResponseDecline = 2;

    void SendMainMenu(Player* player);
    void SendDeadlyTrialMenu(Player* player);
    bool HandleChoiceResponse(Player* player, int32 choiceId, int32 responseId);
}

#endif
