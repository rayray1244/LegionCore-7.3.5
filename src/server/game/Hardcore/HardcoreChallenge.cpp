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

#include "HardcoreChallenge.h"
#include "Player.h"
#include "WorldSession.h"
#include "QuestPackets.h"

namespace HardcoreChallenge
{
    namespace
    {
        void SendChoice(Player* player, int32 choiceId, std::string const& question, std::vector<WorldPackets::Quest::PlayerChoiceResponse> responses)
        {
            if (!player)
                return;

            WorldPackets::Quest::DisplayPlayerChoice displayChoice;
            displayChoice.SenderGUID = player->GetGUID();
            displayChoice.ChoiceID = choiceId;
            displayChoice.Question = question;
            displayChoice.Responses = std::move(responses);
            player->SendDirectMessage(displayChoice.Write());
        }
    }

    void SendMainMenu(Player* player)
    {
        std::vector<WorldPackets::Quest::PlayerChoiceResponse> responses;
        WorldPackets::Quest::PlayerChoiceResponse deadlyTrial;
        deadlyTrial.ResponseID = ResponseOpenDeadlyTrial;
        deadlyTrial.Answer = "Смертельное испытание";
        deadlyTrial.Header = "Режим хардкор";
        deadlyTrial.Description = "Включить или отказаться от режима с одной жизнью.";
        responses.push_back(std::move(deadlyTrial));

        SendChoice(player, ChoiceIdMain, "Меню персонажа", std::move(responses));
    }

    void SendDeadlyTrialMenu(Player* player)
    {
        std::vector<WorldPackets::Quest::PlayerChoiceResponse> responses;

        WorldPackets::Quest::PlayerChoiceResponse enable;
        enable.ResponseID = ResponseEnable;
        enable.Answer = "Включить";
        enable.Header = "Подтверждение";
        enable.Description = "Одна смерть = блокировка входа навсегда.";
        enable.Confirmation = "Ты уверен?";
        responses.push_back(std::move(enable));

        WorldPackets::Quest::PlayerChoiceResponse decline;
        decline.ResponseID = ResponseDecline;
        decline.Answer = "Нет, отказаться";
        decline.Header = "Отказ";
        decline.Description = "Режим не будет включен.";
        responses.push_back(std::move(decline));

        std::string question = "СМЕРТЕЛЬНОЕ ИСПЫТАНИЕ\n"
                               "Правила:\n"
                               "- одна жизнь, смерть навсегда блокирует вход\n"
                               "- смерть в дуэли также считается\n"
                               "- отказаться можно сейчас, позже включить снова через меню\n";

        SendChoice(player, ChoiceIdDeadlyTrial, question, std::move(responses));
    }

    bool HandleChoiceResponse(Player* player, int32 choiceId, int32 responseId)
    {
        if (!player)
            return false;

        if (choiceId == ChoiceIdMain)
        {
            if (responseId == ResponseOpenDeadlyTrial)
            {
                SendDeadlyTrialMenu(player);
                return true;
            }

            return false;
        }

        if (choiceId != ChoiceIdDeadlyTrial)
            return false;

        if (responseId == ResponseEnable)
        {
            if (player->IsHardcore())
            {
                player->GetSession()->SendNotification("Режим хардкор уже включен.");
                return true;
            }

            player->SetHardcore(true);
            player->SaveToDB();
            player->GetSession()->SendNotification("Режим хардкор включен. Удачи!");
            return true;
        }

        if (responseId == ResponseDecline)
        {
            player->GetSession()->SendNotification("Режим хардкор не включен.");
            return true;
        }

        return false;
    }
}
