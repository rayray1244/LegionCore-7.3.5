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

#ifndef LEGION_DAILY_VOW_H
#define LEGION_DAILY_VOW_H

#include <cstdint>

class Creature;
class Player;
class Spell;
class Unit;

namespace DailyVow
{
    enum VowId : uint8_t
    {
        VowNone = 0,
        VowNoExternalHeals = 1,
        VowNoMounts = 2,
        VowKillBeasts = 3
    };

    void SendMenu(Player* player);
    bool HandleChoiceResponse(Player* player, int32 choiceId, int32 responseId);
    void OnLogin(Player* player);
    void OnLogout(Player* player);
    void OnCreatureKill(Player* player, Creature* creature);
    void OnHeal(Unit* healer, Unit* receiver, uint32& gain);
    void OnSpellCast(Player* player, Spell* spell);
}

#endif
