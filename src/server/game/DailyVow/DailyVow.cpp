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

#include "DailyVow.h"
#include "CharacterDatabase.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestPackets.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Timer.h"
#include "World.h"
#include "WorldSession.h"

#include <ctime>
#include <unordered_map>

namespace DailyVow
{
    namespace
    {
        constexpr int32 ChoiceId = 920000;
        constexpr int32 ResponseNoExternalHeals = 1;
        constexpr int32 ResponseNoMounts = 2;
        constexpr int32 ResponseKillBeasts = 3;
        constexpr uint32 KillTargetCount = 50;

        struct State
        {
            uint32 date = 0;
            uint8 vowId = VowNone;
            uint32 progress = 0;
            bool completed = false;
            bool failed = false;
            uint32 lastWarning = 0;
        };

        std::unordered_map<ObjectGuid::LowType, State> vowStates;

        uint32 GetTodayDate()
        {
            time_t now = sWorld->GetGameTime();
            tm localTm;
#if PLATFORM == PLATFORM_WINDOWS
            localtime_s(&localTm, &now);
#else
            localtime_r(&now, &localTm);
#endif
            return (localTm.tm_year + 1900) * 10000 + (localTm.tm_mon + 1) * 100 + localTm.tm_mday;
        }

        std::string GetVowName(uint8 vowId)
        {
            switch (vowId)
            {
                case VowNoExternalHeals:
                    return "Не получать хил от других";
                case VowNoMounts:
                    return "Не использовать маунта";
                case VowKillBeasts:
                    return "Убить 50 зверей";
                default:
                    return "Нет";
            }
        }

        void SaveState(Player* player, State const& state)
        {
            if (!player)
                return;

            CharacterDatabase.PExecute(
                "REPLACE INTO character_daily_vow (guid, date, vow_id, progress, completed, failed) "
                "VALUES (%u, %u, %u, %u, %u, %u)",
                player->GetGUIDLow(),
                state.date,
                state.vowId,
                state.progress,
                state.completed ? 1 : 0,
                state.failed ? 1 : 0);
        }

        State& GetState(Player* player)
        {
            ObjectGuid::LowType guid = player->GetGUIDLow();
            auto itr = vowStates.find(guid);
            if (itr == vowStates.end())
                itr = vowStates.emplace(guid, State{}).first;

            State& state = itr->second;
            uint32 today = GetTodayDate();
            if (state.date != 0 && state.date < today)
            {
                state.date = today;
                state.vowId = VowNone;
                state.progress = 0;
                state.completed = false;
                state.failed = false;
                state.lastWarning = 0;
                SaveState(player, state);
            }

            if (state.date == 0)
                state.date = today;

            return state;
        }

        void Reward(Player* player)
        {
            if (!player)
                return;

            uint32 rewardGold = sWorld->getIntConfig(CONFIG_DAILY_VOW_REWARD_GOLD);
            if (rewardGold)
                player->ModifyMoney(int32(rewardGold));

            if (uint32 rewardSpell = sWorld->getIntConfig(CONFIG_DAILY_VOW_REWARD_SPELL_ID))
                player->CastSpell(player, rewardSpell, true);

            player->GetSession()->SendNotification("Клятва дня выполнена. Награда получена!");
        }

        void SendChoice(Player* player, std::string const& question, std::vector<WorldPackets::Quest::PlayerChoiceResponse> responses)
        {
            WorldPackets::Quest::DisplayPlayerChoice displayChoice;
            displayChoice.SenderGUID = player->GetGUID();
            displayChoice.ChoiceID = ChoiceId;
            displayChoice.Question = question;
            displayChoice.Responses = std::move(responses);
            player->SendDirectMessage(displayChoice.Write());
        }
    }

    void SendMenu(Player* player)
    {
        if (!player)
            return;

        State& state = GetState(player);
        std::string status = "Текущая клятва: " + GetVowName(state.vowId);
        if (state.vowId == VowKillBeasts)
            status += " (" + std::to_string(state.progress) + "/" + std::to_string(KillTargetCount) + ")";
        if (state.failed)
            status += " — провалена";
        if (state.completed)
            status += " — выполнена";

        std::string question = "Клятва дня\n" + status + "\nВыбери одну клятву на сегодня:";

        std::vector<WorldPackets::Quest::PlayerChoiceResponse> responses;
        WorldPackets::Quest::PlayerChoiceResponse noHeals;
        noHeals.ResponseID = ResponseNoExternalHeals;
        noHeals.Answer = "Не получать хил от других";
        noHeals.Header = "Клятва: Стойкость";
        noHeals.Description = "Любой хил от других игроков проваливает клятву.";
        responses.push_back(std::move(noHeals));

        WorldPackets::Quest::PlayerChoiceResponse noMounts;
        noMounts.ResponseID = ResponseNoMounts;
        noMounts.Answer = "Не использовать маунта";
        noMounts.Header = "Клятва: Пеший путь";
        noMounts.Description = "Любая попытка оседлать маунта проваливает клятву.";
        responses.push_back(std::move(noMounts));

        WorldPackets::Quest::PlayerChoiceResponse killBeasts;
        killBeasts.ResponseID = ResponseKillBeasts;
        killBeasts.Answer = "Убить 50 зверей";
        killBeasts.Header = "Клятва: Охота";
        killBeasts.Description = "Убей 50 существ типа 'зверь' за день.";
        responses.push_back(std::move(killBeasts));

        SendChoice(player, question, std::move(responses));
    }

    bool HandleChoiceResponse(Player* player, int32 choiceId, int32 responseId)
    {
        if (!player || choiceId != ChoiceId)
            return false;

        State& state = GetState(player);
        if (state.vowId != VowNone)
        {
            player->GetSession()->SendNotification("Клятва дня уже выбрана.");
            return true;
        }

        state.date = GetTodayDate();
        state.progress = 0;
        state.completed = false;
        state.failed = false;

        switch (responseId)
        {
            case ResponseNoExternalHeals:
                state.vowId = VowNoExternalHeals;
                break;
            case ResponseNoMounts:
                state.vowId = VowNoMounts;
                break;
            case ResponseKillBeasts:
                state.vowId = VowKillBeasts;
                break;
            default:
                return true;
        }

        SaveState(player, state);
        player->GetSession()->SendNotification("Клятва дня активирована: %s.", GetVowName(state.vowId).c_str());
        return true;
    }

    void OnLogin(Player* player)
    {
        if (!player)
            return;

        State state;
        if (QueryResult result = CharacterDatabase.PQuery("SELECT date, vow_id, progress, completed, failed FROM character_daily_vow WHERE guid = %u", player->GetGUIDLow()))
        {
            Field* fields = result->Fetch();
            state.date = fields[0].GetUInt32();
            state.vowId = fields[1].GetUInt8();
            state.progress = fields[2].GetUInt32();
            state.completed = fields[3].GetBool();
            state.failed = fields[4].GetBool();
        }
        else
            state.date = GetTodayDate();

        uint32 today = GetTodayDate();
        if (state.date != 0 && state.date < today)
        {
            bool reward = false;
            if (state.vowId == VowKillBeasts)
                reward = !state.failed && state.progress >= KillTargetCount;
            else if (state.vowId != VowNone)
                reward = !state.failed;

            if (reward)
                Reward(player);

            state.date = today;
            state.vowId = VowNone;
            state.progress = 0;
            state.completed = false;
            state.failed = false;
        }

        vowStates[player->GetGUIDLow()] = state;
        SaveState(player, state);
    }

    void OnLogout(Player* player)
    {
        if (!player)
            return;

        vowStates.erase(player->GetGUIDLow());
    }

    void OnCreatureKill(Player* player, Creature* creature)
    {
        if (!player || !creature)
            return;

        State& state = GetState(player);
        if (state.vowId != VowKillBeasts || state.failed || state.completed)
            return;

        if (creature->GetCreatureType() != CREATURE_TYPE_BEAST)
            return;

        ++state.progress;
        if (state.progress >= KillTargetCount)
        {
            state.completed = true;
            SaveState(player, state);
            Reward(player);
        }
        else
            SaveState(player, state);
    }

    void OnHeal(Unit* healer, Unit* receiver, uint32& gain)
    {
        if (!healer || !receiver)
            return;

        Player* player = receiver->ToPlayer();
        if (!player)
            return;

        State& state = GetState(player);
        if (state.vowId != VowNoExternalHeals || state.failed || state.completed)
            return;

        if (healer == receiver)
            return;

        gain = 0;
        state.failed = true;
        SaveState(player, state);

        uint32 now = getMSTime();
        if (now - state.lastWarning > 5000)
        {
            state.lastWarning = now;
            player->GetSession()->SendNotification("Клятва дня провалена: получен хил от другого игрока.");
        }
    }

    void OnSpellCast(Player* player, Spell* spell)
    {
        if (!player || !spell)
            return;

        State& state = GetState(player);
        if (state.vowId != VowNoMounts || state.failed || state.completed)
            return;

        SpellInfo const* info = spell->GetSpellInfo();
        if (!info || !info->IsMountOrCompanions())
            return;

        spell->cancel();
        state.failed = true;
        SaveState(player, state);
        player->GetSession()->SendNotification("Клятва дня провалена: попытка использовать маунта.");
    }
}
