/*
* Copyright 2009, 2010, The Pink Petal Development Team.
* The Pink Petal Devloment Team are defined as the game's coders
* who meet on http://pinkpetal.org     // old site: http://pinkpetal .co.cc
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma region //	Includes and Externs			//
#include "cRng.h"
#include "src/buildings/cBrothel.h"
#include "cInventory.h"
#include "src/Game.hpp"

extern cRng g_Dice;

#pragma endregion

// `J` Job arena - Staff
bool cJobManager::WorkCobbler(sGirl* girl, bool Day0Night1, string& summary)
{
    auto brothel = girl->m_Building;
#pragma region //	Job setup				//
	int actiontype = ACTION_WORKMAKEITEMS;
	stringstream ss; string girlName = girl->m_Realname; ss << girlName;
	int roll_a = g_Dice.d100(), roll_b = g_Dice.d100(), roll_c = g_Dice.d100();
	if (girl->disobey_check(actiontype, JOB_COBBLER))			// they refuse to work
	{
		ss << " refused to work during the " << (Day0Night1 ? "night" : "day") << " shift.";
		girl->m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
		return true;
	}
	ss << " worked making shoes and other leather items at the arena.\n \n";

	cGirls::UnequipCombat(girl);	// put that shit away, you'll scare off the customers!

	int wages = 20, tips = 0;
	int enjoy = 0;
	int imagetype = IMGTYPE_CRAFT;
	int msgtype = Day0Night1;

#pragma endregion
#pragma region //	Job Performance			//

	double jobperformance = JP_Cobbler(girl, false);
	double craftpoints = jobperformance/10;

	int dirtyloss = brothel->m_Filthiness / 10;		// craftpoints lost due to repairing equipment
	if (dirtyloss > 0)
	{
		craftpoints -= dirtyloss * 2;
		brothel->m_Filthiness -= dirtyloss;
		ss << "She spent some of her time repairing the Arena's equipment instead of making new stuff.\n";
	}

	if (jobperformance >= 245)
	{
		ss << " She must be the perfect at this.";
		craftpoints *= 1.2; roll_a += 10; roll_b += 25;
	}
	else if (jobperformance >= 185)
	{
		ss << " She's unbelievable at this.";
		craftpoints *= 1.1; roll_a += 5; roll_b += 18;
	}
	else if (jobperformance >= 145)
	{
		ss << " She's good at this job.";
		roll_a += 2; roll_b += 10;
	}
	else if (jobperformance >= 100)
	{
		ss << " She made a few mistakes but overall she is okay at this.";
		craftpoints *= 0.8;
	}
	else if (jobperformance >= 70)
	{
		ss << " She was nervous and made a few mistakes. She isn't that good at this.";
		craftpoints *= 0.6; roll_a -= 2; roll_b -= 5;
	}
	else
	{
		ss << " She was nervous and constantly making mistakes. She really isn't very good at this job.";
		wages -= 10; craftpoints *= 0.4; roll_a -= 5; roll_b -= 10;
	}
	ss << "\n \n";

#pragma endregion
#pragma region	//	Enjoyment and Tiredness		//


	int tired = (300 - (int)jobperformance);	// this gets divided in roll_a by (10, 12 or 14) so it will end up around 0-23 tired
	if (roll_a <= 10)
	{
		tired /= 10;
		enjoy -= g_Dice % 3;
		if (roll_b < 20)	// injury
		{
			girl->health(-(1 + g_Dice % 5));
			craftpoints *= 0.8;
			if (girl->magic() > 50 && girl->mana() > 20)
			{
				girl->mana(-10 - (g_Dice % 10));
				ss << "While trying to enchant an item, the magic rebounded on her";
			}
			else
				ss << "She hurt herself while making items";
			if (girl->is_dead())
			{
				ss << " killing her.";
				g_Game.push_message(girlName + " was killed in an accident while working as a Cobbler at the Arena.", COLOR_RED);
				return false;	// not refusing, she is dead
			}
			else ss << ".";
		}

		else	// unhappy
		{
			ss << "She did not like";
			if (g_Dice % 2)	ss << " making shoes today.";
			else ss << " working with animal hides today.";
			girl->happiness(-(g_Dice % 11));
		}
	}
	else if (roll_a >= 90)
	{
		tired /= 14;
		craftpoints *= 1.1;
		enjoy += g_Dice % 3;
		/* */if (roll_b < 50)	ss << "She kept a steady pace with her neddle work by humming a pleasant tune.";
		else /*            */	ss << "She had a great time working today.";
	}
	else
	{
		tired /= 12;
		enjoy += g_Dice % 2;
		ss << "The shift passed uneventfully.";
	}
	ss << "\n \n";

#pragma endregion
#pragma region	//	Money					//


	// slave girls not being paid for a job that normally you would pay directly for do less work
	if ((girl->is_slave() && !cfg.initial.slave_pay_outofpocket()))
	{
		craftpoints *= 0.9;
		wages = 0;
	}
	else
	{
		wages += int(craftpoints); // `J` Pay her based on how much she made
	}

#pragma endregion
#pragma region	//	Create Items				//

    if (craftpoints > 0)
    {
        int points_remaining = (int)craftpoints;
        int numitems = 0;

        while (points_remaining > 0 && numitems < (1 + girl->crafting() / 15))
        {
            auto item = g_Game.inventory_manager().GetCraftableItem(*girl, JOB_COBBLER, points_remaining);
            if(!item) {
                // try something easier. Get craftable item does not return items which need less than
                // points_remaining / 3 crafting points
                item = g_Game.inventory_manager().GetCraftableItem(*girl, JOB_COBBLER, points_remaining / 2);
            }
            if(!item) {
                points_remaining -= 10;
                continue;
            }

            points_remaining -= item->m_Crafting.craft_cost();
            girl->mana(-item->m_Crafting.mana_cost());
            msgtype = EVENT_GOODNEWS;
            if (numitems == 0)	ss << "\n \n" << girlName << " made:";
            ss << "\n" << item->m_Name;
            g_Game.player().inventory().add_item(item);
            numitems++;
        }
    }

#pragma endregion
#pragma region	//	Finish the shift			//


	// `J` - Finish the shift - Cobbler

	// Push out the turn report
	girl->m_Events.AddMessage(ss.str(), imagetype, msgtype);
	if (tired > 0) girl->tiredness(tired);

	// Money
	girl->m_Tips = max(0, tips);
	girl->m_Pay = max(0, wages);

	// Base Improvement and trait modifiers
	int xp = 5, libido = 1, skill = 3;
	/* */if (girl->has_trait("Quick Learner"))	{ skill += 1; xp += 3; }
	else if (girl->has_trait("Slow Learner"))	{ skill -= 1; xp -= 3; }
	/* */if (girl->has_trait("Nymphomaniac"))	{ libido += 2; }
	// EXP and Libido
	girl->exp((g_Dice % xp) + 1);
	girl->upd_temp_stat(STAT_LIBIDO, libido,false);

	// primary improvement (+2 for single or +1 for multiple)
	girl->upd_skill(SKILL_CRAFTING,	(g_Dice % skill) + 1);
	girl->upd_stat( STAT_STRENGTH, (g_Dice % skill) + 1);
	// secondary improvement (-1 for one then -2 for others)
	girl->upd_stat( STAT_CONSTITUTION,	max(0, (g_Dice % skill) - 1));
	girl->upd_skill(SKILL_COMBAT, max(0, (g_Dice % skill) - 2));

	// Update Enjoyment
	girl->upd_Enjoyment(actiontype, enjoy);

#pragma endregion
	return false;
}

double cJobManager::JP_Cobbler(sGirl* girl, bool estimate)// not used
{
	double jobperformance =
		// primary - first 100
		girl->crafting() +
		// secondary - second 100
		((girl->service() + girl->intelligence() + girl->magic()) / 3) +
		// level bonus
		girl->level();
		// traits modifiers
	if (!estimate)
	{
		int t = girl->tiredness() - 80;
		if (t > 0)
			jobperformance -= (t + 2) * (t / 3);
	}

	return jobperformance;
}
