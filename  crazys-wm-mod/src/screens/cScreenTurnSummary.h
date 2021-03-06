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
#pragma once

#include "DirPath.h"
#include "cInterfaceWindow.h"
#include "InterfaceGlobals.h"

class cScreenTurnSummary : public cInterfaceWindowXML
{
private:
	int brothel_id;
	int gold_id;			// Player Gold
	int category_id;
	int labelitem_id;
	int item_id;
	int event_id;
	int labeldesc_id;
	int goto_id;
	int nextweek_id;
	int back_id;
	int prev_id;
	int next_id;
	int image_id;
	int imagename_id;

    enum SummaryCategory {
        Summary_GIRLS,
        Summary_GANGS,
        Summary_BUILDINGS,
        Summary_DUNGEON,
    };

    SummaryCategory m_ActiveCategory;

    void set_ids() override;
    void OnKeyPress(SDL_keysym key) override;
public:
	cScreenTurnSummary();

	void init(bool back) override;
	void process() override;

    void Fill_Items_GANGS();
	void Fill_Items_BUILDINGS();
	void Fill_Items_DUNGEON();
    void Fill_Items_Building(IBuilding * building);

	void Fill_Events(sGirl* girl);
	void Fill_Events_GANGS();
	void Fill_Events_BROTHELS();

    void change_category(SummaryCategory);

    void change_item();

    void change_event();

    void goto_selected();
};