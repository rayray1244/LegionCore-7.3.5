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

#include "DailyVow/DailyVow.h"
#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "World.h"

class daily_vow_player : public PlayerScript
{
public:
    daily_vow_player() : PlayerScript("daily_vow_player") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        DailyVow::OnLogin(player);
    }

    void OnLogout(Player* player) override
    {
        DailyVow::OnLogout(player);
    }

    void OnCreatureKill(Player* killer, Creature* killed) override
    {
        DailyVow::OnCreatureKill(killer, killed);
    }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell)
            return;

        uint32 menuSpellId = sWorld->getIntConfig(CONFIG_DAILY_VOW_MENU_SPELL_ID);
        if (menuSpellId && spell->GetSpellInfo()->Id == menuSpellId)
        {
            DailyVow::SendMenu(player);
            spell->cancel();
            return;
        }

        DailyVow::OnSpellCast(player, spell);
    }
};

class daily_vow_unit : public UnitScript
{
public:
    daily_vow_unit() : UnitScript("daily_vow_unit") { }

    void OnHeal(Unit* healer, Unit* receiver, uint32& gain) override
    {
        DailyVow::OnHeal(healer, receiver, gain);
    }
};

void AddSC_daily_vow()
{
    new daily_vow_player();
    new daily_vow_unit();
}
