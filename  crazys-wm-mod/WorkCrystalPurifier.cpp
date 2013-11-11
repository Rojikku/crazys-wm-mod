/*
 * Copyright 2009, 2010, The Pink Petal Development Team.
 * The Pink Petal Devloment Team are defined as the game's coders 
 * who meet on http://pinkpetal.co.cc
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
#include "cJobManager.h"
#include "cBrothel.h"
#include "cClinic.h"
#include "cMovieStudio.h"
#include "cCustomers.h"
#include "cRng.h"
#include "cInventory.h"
#include "sConfig.h"
#include "cRival.h"
#include <sstream>
#include "CLog.h"
#include "cTrainable.h"
#include "cTariff.h"
#include "cGold.h"
#include "cGangs.h"
#include "cMessageBox.h"
#include "libintl.h"

extern cRng g_Dice;
extern CLog g_LogFile;
extern cCustomers g_Customers;
extern cInventory g_InvManager;
extern cBrothelManager g_Brothels;
extern cMovieStudioManager g_Studios;
extern cGangManager g_Gangs;
extern cMessageQue g_MessageQue;

bool cJobManager::WorkCrystalPurifier(sGirl* girl, sBrothel* brothel, int DayNight, string& summary)
{
	string message = "";
	char buffer[1000];
	int jobperformance = 0;
	string girlName = girl->m_Realname;

	g_Girls.UnequipCombat(girl);
	
	girl->m_Pay += 20;
	
	message = girlName;	
	message += gettext(" worked as a crystal purifier.\n\n");
	
	int roll = g_Dice%100;
	if (roll <= 10 && g_Girls.DisobeyCheck(girl, ACTION_WORKMOVIE, brothel))
	{
		message = girl->m_Realname + gettext(" refused to work as a crystal purifier today.");

		girl->m_Events.AddMessage(message, IMGTYPE_PROFILE, EVENT_NOWORK);
		return true;
	}
	else if(roll <= 15) {
		message += gettext(" She did not like working in the studio today.\n\n");
		g_Girls.UpdateEnjoyment(girl, ACTION_WORKMOVIE, -1, true);
		jobperformance += -5;
	}
	else if(roll >=90)
	{
		message += gettext(" She had a great time working today.\n\n");
		g_Girls.UpdateEnjoyment(girl, ACTION_WORKMOVIE, +3, true);
		jobperformance += 5;
	}
	else
	{
		message += gettext(" Otherwise, the shift passed uneventfully.\n\n");
		g_Girls.UpdateEnjoyment(girl, ACTION_WORKMOVIE, +1, true);
	}

/*
 *	work out the pay between the house and the girl
 *
 *	the original calc took the average of beauty and charisma and halved it
 */
	int roll_max = girl->spirit() + girl->intelligence();
	roll_max /= 4;
	girl->m_Pay += 10 + g_Dice%roll_max;
	
	jobperformance += (girl->spirit() - 50) / 10;
	jobperformance += (girl->intelligence() - 50) / 10;
	jobperformance += g_Girls.GetSkill(girl, SKILL_SERVICE) / 10;
	jobperformance /= 3;
	jobperformance += g_Girls.GetStat(girl, STAT_LEVEL);
	jobperformance += g_Dice%4 - 1;	// should add a -1 to +3 random element --PP
	
	g_Studios.m_PurifierQaulity += jobperformance;

	if(jobperformance > 0)
	{
	message += girlName + gettext(" helped improve the scene ");
						_itoa(jobperformance,buffer,10);
						message += buffer;
						message += "% with her production skills. \n";
	}
	else if(jobperformance < 0)
	{
	message += girlName + gettext(" did a bad job today, she reduced the scene quality ");
					_itoa(jobperformance,buffer,10);
					message += buffer;
					message += "% with her poor performance. \n";
	}
	else
		message += girlName + gettext(" did not really help the scene quality.\n");

	// Improve stats
	int xp = 5, skill = 3;

	if (g_Girls.HasTrait(girl, "Quick Learner"))
	{
		skill += 1;
		xp += 3;
	}
	else if (g_Girls.HasTrait(girl, "Slow Learner"))
	{
		skill -= 1;
		xp -= 3;
	}
	g_Girls.UpdateSkill(girl, SKILL_SERVICE, skill);
	g_Girls.UpdateStat(girl, STAT_EXP, xp);
	girl->m_Events.AddMessage(message, IMGTYPE_PROFILE, DayNight);

	return false;
}
