/*
 * Copyright (C) 2008-2012 TrinityCore <https://www.trinitycore.org/>
 *
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

#include "Hardcore/HardcoreChallenge.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "World.h"

class hardcore_challenge_player : public PlayerScript
{
public:
    hardcore_challenge_player() : PlayerScript("hardcore_challenge_player") { }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell)
            return;

        uint32 menuSpellId = sWorld->getIntConfig(CONFIG_HARDCORE_MENU_SPELL_ID);
        if (!menuSpellId || spell->GetSpellInfo()->Id != menuSpellId)
            return;

        HardcoreChallenge::SendMainMenu(player);
        spell->cancel();
    }
};

void AddSC_hardcore_challenge()
{
    new hardcore_challenge_player();
}
