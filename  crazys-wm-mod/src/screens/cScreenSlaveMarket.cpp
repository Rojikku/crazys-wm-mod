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
#include <vector>
#include "buildings/cBrothel.h"
#include "main.h"
#include "cScreenSlaveMarket.h"
#include "cWindowManager.h"
#include "cGold.h"
#include "cTariff.h"
#include "Game.hpp"

extern bool g_Cheats;

extern	bool	g_AltKeys;	// New hotkeys --PP
extern	bool	g_ShiftDown;
extern	bool	g_S_Key;

cTariff tariff;
cScreenSlaveMarket::cScreenSlaveMarket() : cInterfaceWindowXML("slavemarket_screen.xml")
{
    m_SelectedGirl = -1;
	ImageNum       = -1;
	DetailLevel    = 0;
	sel_pos        = 0;
}

void cScreenSlaveMarket::set_ids()
{
	back_id				/**/ = get_id("BackButton", "Back");
	more_id				/**/ = get_id("ShowMoreButton");
	buy_slave_id		/**/ = get_id("BuySlaveButton");
	cur_brothel_id		/**/ = get_id("CurrentBrothel","*Unused*");//
	slave_list_id		/**/ = get_id("SlaveList");
	trait_list_id		/**/ = get_id("TraitList","*Unused*");//
	trait_list_text_id	/**/ = get_id("TraitListT");
	details_id			/**/ = get_id("SlaveDetails");
	trait_id			/**/ = get_id("TraitDesc","*Unused*");//
	girl_desc_id		/**/ = get_id("GirlDesc");
	image_id			/**/ = get_id("GirlImage");
	header_id			/**/ = get_id("ScreenHeader","*Unused*");//
	gold_id				/**/ = get_id("Gold", "*Unused*");
	slave_market_id		/**/ = get_id("SlaveMarket");

	releaseto_id		/**/ = get_id("ReleaseTo");
	roomsfree_id		/**/ = get_id("RoomsFree");

    // set up structure with all "release girl to ... " buttons
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Brothel0"), BuildingType::BROTHEL, 0});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Brothel1"), BuildingType::BROTHEL, 1});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Brothel2"), BuildingType::BROTHEL, 2});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Brothel3"), BuildingType::BROTHEL, 3});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Brothel4"), BuildingType::BROTHEL, 4});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Brothel5"), BuildingType::BROTHEL, 5});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Brothel6"), BuildingType::BROTHEL, 6});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("House"), BuildingType::HOUSE, 0});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Clinic"), BuildingType::CLINIC, 0});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Studio"), BuildingType::STUDIO, 0});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Arena"), BuildingType::ARENA, 0});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Centre"), BuildingType::CENTRE, 0});
    m_ReleaseButtons.emplace_back(RelBtnData{get_id("Farm"), BuildingType::FARM, 0});
	/// TODO cannot handle dungeon like the others at the moment
	dungeon_id			/**/ = get_id("Dungeon");

	// set button callbacks
	SetButtonNavigation(back_id, "<back>");
	SetButtonCallback(buy_slave_id, [this](){
	    buy_slaves();
	    init(false);
	});
	SetButtonHotKey(buy_slave_id, SDLK_SPACE);

	for(const auto& btn: m_ReleaseButtons) {
	    SetButtonCallback(btn.id, [this, btn](){ change_release(btn.type, btn.index); });
	}

	SetButtonCallback(dungeon_id, [this]() {
        m_TargetBuilding = nullptr;
        EditTextItem("Send Girl to: The Dungeon", releaseto_id);
        EditTextItem("", roomsfree_id);
	});

    SetButtonCallback(more_id, [this]() {
        sGirl *girl = g_Game.GetMarketSlave(m_SelectedGirl);
        if (DetailLevel == 0)		{ DetailLevel = 1; EditTextItem(cGirls::GetMoreDetailsString(girl, true), details_id); }
        else if (DetailLevel == 1)	{ DetailLevel = 2; EditTextItem(cGirls::GetThirdDetailsString(girl), details_id); }
        else						{ DetailLevel = 0; EditTextItem(cGirls::GetDetailsString(girl, true), details_id); }
    });

    SetListBoxSelectionCallback(slave_list_id, [this](int sel) { change_selected_girl(sel); });
    SetListBoxSelectionCallback(trait_list_id, [this](int sel) { on_select_trait(sel); });
    SetListBoxHotKeys(slave_list_id, g_AltKeys ? SDLK_a : SDLK_UP, g_AltKeys ? SDLK_d : SDLK_DOWN);
}

void cScreenSlaveMarket::init(bool back)
{
	Focused();
	stringstream ss;

	//buttons enable/disable
	DisableButton(more_id, true);
	DisableButton(buy_slave_id, true);
    m_SelectedGirl = -1;

	ImageNum = -1;
	if (cur_brothel_id >= 0)	EditTextItem(active_building().name(), cur_brothel_id);

	change_release(BuildingType::BROTHEL, 0);

    for(const auto& btn: m_ReleaseButtons) {
        HideButton(btn.id, g_Game.buildings().num_buildings(btn.type) < btn.index + 1);
    }

	ClearListBox(slave_list_id);	// clear the list

	for(std::size_t i = 0; i < g_Game.GetNumMarketSlaves(); ++i) {
        int col = COLOR_BLUE;
        if (g_Game.IsMarketUniqueGirl(i))
        {
            col = COLOR_RED;
        }
        AddToListBox(slave_list_id, i, g_Game.GetMarketSlave(i)->m_Realname, col);
	}

    m_SelectedGirl = 0;

	if (header_id >= 0)
	{
		ss.str(""); ss << "Slave Market, " << g_Game.gold().sval() << " gold";
		EditTextItem(ss.str(), header_id);
	}
	if (gold_id >= 0)
	{
		ss.str(""); ss << "Gold: " << g_Game.gold().sval();
		EditTextItem(ss.str(), gold_id);
	}

	// Finds the first girl in the selection, so she is highlighted. This stops the No girl selected that was normal before. --PP
	m_SelectedGirl = g_Game.GetNumMarketSlaves() - 1;

	// if there is still as selection (a non empty slave list) then highlight the current selection
	if (m_SelectedGirl >= 0) SetSelectedItemInList(slave_list_id, m_SelectedGirl, true);
	// now we need to populate the trait box
	if (trait_list_id >= 0) ClearListBox(trait_list_id);
	if (trait_list_text_id >= 0) EditTextItem("Traits:", trait_list_text_id);
	if (girl_desc_id >= 0)	EditTextItem("", girl_desc_id);
	int tmp = GetLastSelectedItemFromList(slave_list_id);
	// if the last item was -1 I assume the list is empty so we can go home early (and probably should have earlier still)
	if (tmp == -1) return;
	// get the girl under the cursor.
	preparescreenitems(g_Game.GetMarketSlave(tmp));
}

bool cScreenSlaveMarket::check_keys()
{
	if (g_AltKeys && g_S_Key)
	{
		sGirl *girl = g_Game.GetMarketSlave(m_SelectedGirl);
		g_S_Key = false;
		if (g_ShiftDown)
		{
			DetailLevel = 2;
			EditTextItem(cGirls::GetThirdDetailsString(girl), details_id);
		}
		else
		{
			if (DetailLevel == 0)		{ DetailLevel = 1; EditTextItem(cGirls::GetMoreDetailsString(girl, true), details_id); }
			else						{ DetailLevel = 0; EditTextItem(cGirls::GetDetailsString(girl, true), details_id); }
		}
		return true;
	}
	return false;
}


void cScreenSlaveMarket::on_select_trait(int selection)
{
    if (selection != -1 && m_SelectedGirl != -1 && g_Game.GetMarketSlave(m_SelectedGirl)->m_NumTraits > selection)
    {
        EditTextItem(g_Game.GetMarketSlave(m_SelectedGirl)->m_Traits[selection]->desc(), trait_id);
    }
    else EditTextItem("", trait_id);
}

void cScreenSlaveMarket::process()
{
	if (check_keys()) return;						// handle arrow keys
	HideImage(image_id, (m_SelectedGirl < 0));		// hide/show image based on whether a girl is selected
	if (m_SelectedGirl < 0)								// if no girl is selected, clear girl info
	{
		EditTextItem("No girl selected", details_id);
		if (trait_id >= 0) EditTextItem("", trait_id);
	}
	// nothing selected == nothing further to do
	int index = selected_item();
	if (index == -1) return;
	sGirl* girl = g_Game.GetMarketSlave(m_SelectedGirl);
	if (!girl) g_LogFile.os() << "... no girl at index " << index << "- returning " << endl;
}

bool cScreenSlaveMarket::buy_slaves()
{
	stringstream ss;
	stringstream sendtotext;

	// set the brothel
	if (m_TargetBuilding == nullptr) sendtotext << "the Dungeon";
	else
	{
	    switch(m_TargetBuilding->type()) {
	    case BuildingType::BROTHEL:
            sendtotext << "your brothel: " << m_TargetBuilding->name();
	        break;
	    case BuildingType::ARENA:
            sendtotext << "the Arena";
            break;
        case BuildingType::STUDIO:
            sendtotext << "the Studio";
            break;
        case BuildingType::CENTRE:
            sendtotext << "the Community Centre";
            break;
        case BuildingType::FARM:
            sendtotext << "the Farm";
            break;
        case BuildingType::CLINIC:
            sendtotext << "the Clinic";
            break;
        case BuildingType::HOUSE:
            sendtotext << "your House";
            break;
        }
	}

	// set the girls
	int numgirls;

	int totalcost = 0;
	std::vector<sGirl*> girls_bought;
	for (int sel = multi_slave_first(); sel != -1; sel = multi_slave_next())
	{
		auto girl = g_Game.GetMarketSlave(sel);
		girls_bought.push_back(girl);
		totalcost += tariff.slave_buy_price(girl);
	}
	numgirls = girls_bought.size();

	// Check if there is enough room where we want to send her
	if (m_TargetBuilding)	// Dungeon has no limit so don't bother checking if sending them there.
	{
		if (m_TargetBuilding->free_rooms() < 0)
		{
			g_Game.push_message("The current building has no more room.\nChoose another building to send them to.", 0);
			return false;
		}
		if (m_TargetBuilding->free_rooms() < numgirls)
		{
			g_Game.push_message("The current building does not have enough room for all the girls you want to send there.\nChoose another building or select fewer girls at a time to buy.", 0);
			return false;
		}
	}

	// `J` check if we can afford all the girls selected
	if (!g_Game.gold().slave_cost(totalcost))	// this pays for them if you can afford them
	{									// otherwise this part runs and returns a fail message.
		stringstream text;
		if (numgirls > 4 && g_Game.gold().ival() < totalcost / 2) text << "Calm down!  ";
		text << "You don't have enough money to purchase ";
		switch (numgirls)
		{
		case 0: text << "... noone ... Wait? What? Something went wrong.\n\n Please report this to the Pink Petal Devloment Team at http://pinkpetal.org"; break;
		case 1: text << girls_bought.front()->m_Realname; break;
		case 2: text << "these two"; break;
		case 3: text << "these three"; break;
		default: text << numgirls; break;
		}
		text << (numgirls <= 1 ? "" : " girls") << ".";
		g_Game.push_message(text.str(), 0);
		return false;
	}

	// `J` we could afford the girls so lets get to it
	ss << "You buy ";

	/* */if (numgirls == 1)	ss << "a girl,   " << girls_bought.front()->m_Realname << "   and send her to " << sendtotext.str();
	else if (numgirls == 2)	ss << "two girls,   " << girls_bought.front()->m_Realname << "   and   " << girls_bought[1]->m_Realname << ". You send them to " << sendtotext.str();
	else /*              */	ss << numgirls << " girls and send them to " << sendtotext.str();
	ss << ".\n\n";

	// `J` zzzzzz - add in flavor texts here
	if (numgirls < 1) ss << "(error, no girls)";

	else if (!m_TargetBuilding) {
#pragma region //	Send them to the Dungeon		//
        if (g_Game.player().disposition() >= 80)                //Benevolent
        {
            ss << "\"Don't worry " << (numgirls == 1 ? "my dear" : "ladies")
               << ", I'm only sending you there until I find room for you somewhere better.\"\n";
        } else if (g_Game.player().disposition() >= 50)            // nice
        {
            ss << "\"Don't worry " << (numgirls == 1 ? "my dear" : "ladies")
               << ", you will not be in there long, I promise.\"\n";
        } else if (g_Game.player().disposition() >= 10)            //Pleasant
        {
            ss << "\"Try to make " << (numgirls == 1 ? "yourself" : "yourselves")
               << " comfortable, you should not be in there too long.\"\n";
        } else if (g_Game.player().disposition() >= -10)            // neutral
        {
            ss << "\"To the Dungeon with " << (numgirls == 1 ? "you" : "them") << ", I'll see you there shortly.\"\n";
        } else if (g_Game.player().disposition() >= -50)            // not nice
        {
            ss << "\"To the Dungeon with " << (numgirls == 1 ? "you" : "them")
               << ". I'll enjoy this, but you may not.\"\n";

        } else if (g_Game.player().disposition() >= -80)            //Mean
        {
            ss << "\"To the Dungeon with " << (numgirls == 1 ? "you" : "them") << ". Put "
               << (numgirls == 1 ? "her" : "them") << " in chains and I'll get to them when I am done here.\"\n";
        } else/*                                        */    //Evil
        {
            ss
                    << "\"You are off to your new home, the Dungeon, where my dreams happen and your dreams become nightmares.\"\n";

        }
#pragma endregion
#pragma region //	Send them to a Brothel			//
    }
	else if (m_TargetBuilding->type() == BuildingType::BROTHEL)
	{
		if (g_Game.player().disposition() >= 80)				// Benevolent
		{
			if (numgirls == 1)
			{
				/* */if (g_Dice % 2)	ss << "She went to your current brothel with a smile on her face happy that such a nice guy bought her.";
				else/*            */	ss << "She smiled as you offered her your arm, surprised to find such a kindness waiting for her. Hopeing such kindness would continue, she went happily with you as her new owner.";
			}
			else
			{
				/* */if (g_Dice % 2)	ss << "They went to your current brothel with smiles on their faces, happy that such a nice guy bought them.";
				else if (g_Dice % 2)	ss << "They smiled as you offered them your arm, surprised to find such a kindness waiting for them. Hopeing such kindness would continue, they went happily with you as their new owner.";
				else/*            */	ss << "The crowds chear and congradulate each of the girls that you buy as they walk off the stage and into your custody.";
			}
		}
		else if (g_Game.player().disposition() >= 50)			// Nice
		{
			if (numgirls == 1)
			{
				/* */if (g_Dice % 2)	ss << "Having heard about her new owner's reputation, she was guided to your current brothel without giving any trouble.";
				else/*            */	ss << "She looked up at you hopefully as you refused the use of a retainer or delivery, instead finding herself taken into your retinue for the day and given a chance to enjoy the fresh air before you bring her to her new home.";
			}
			else
			{
				/* */if (g_Dice % 2)	ss << "Having heard about their new owner's reputation, they were guided to your current brothel without giving any trouble.";
				else/*            */	ss << "They looked up at you hopefully as you refused the use of a retainer or delivery, instead finding themselves taken into your retinue for the day and given a chance to enjoy the fresh air before you bring them to their new home.";
			}
		}
		else if (g_Game.player().disposition() >= 10)			// Pleasant
		{
			if (numgirls == 1)
			{
				/* */if (g_Dice % 2)	ss << "She was sent to your current brothel, knowing that she could have been bought by a lot worse owner.";
				else/*            */	ss << "She was escorted home by one of your slaves who helped her settle in. She seems rather hopeful of a good life in your care.";
			}
			else
			{
				/* */if (g_Dice % 2)	ss << "They were sent to your current brothel, knowing that they could have been bought by a lot worse owner.";
				else/*            */	ss << "They were escorted home by one of your slaves who helped them settle in. They seem rather hopeful of a good life in your care.";
			}
		}
		else if (g_Game.player().disposition() >= -10)			// Neutral
		{
			if (numgirls == 1)
			{
				/* */if (g_Dice % 2)	ss << "As your newest investment, she was sent to your current brothel.";
				else/*            */	ss << "She has been sent to your establishment under the supervision of your most trusted slaves.";
			}
			else
			{
				/* */if (g_Dice % 2)	ss << "As your newest investments, they were sent to your current brothel.";
				else/*            */	ss << "They have been sent to your establishment under the supervision of your most trusted slaves.";
			}
		}
		else if (g_Game.player().disposition() >= -50)			// Not nice
		{
			if (numgirls == 1)
			{
				/* */if (g_Dice % 2)	ss << "Not being very happy about her new owner, she was escorted to your current brothel.";
				else/*            */	ss << "She struggled as her hands were shackled in front of her. Her eyes locked on the floor, tears gathering in the corners of her eyes, as she was sent off to your brothel.";
			}
			else
			{
				/* */if (g_Dice % 2)	ss << "Not being very happy about their new owner, they were escorted to your current brothel.";
				else/*            */	ss << "They struggled as their hands were shackled in front of them. With their eyes locked on the floor and tears gathering in the corners of their eyes, they were sent off to your brothel.";
			}
		}
		else if (g_Game.player().disposition() >= -80)			//Mean
		{
			if (numgirls == 1)
			{
				/* */if (g_Dice % 2)	ss << "She didn't want to provoke you in any possible way. She went quietly to your current brothel, without any resistance.";
				else/*            */	ss << "She was dragged away to your brothel crying, one of your guards slapping her face as she tried to resist.";
			}
			else
			{
				/* */if (g_Dice % 2)	ss << "They didn't want to provoke you in any possible way. They went quietly to your current brothel, without any resistance.";
				else/*            */	ss << "They were dragged away to your brothel crying, some of them getting slapped in the face when she tries to resist.";
			}
		}
		else											// Evil
		{
			if (numgirls == 1)
			{
				/* */if (g_Dice % 2)	ss << "She was dragged crying and screaming to your current brothel afraid of what you might do to her as her new owner.";
				else/*            */	ss << "She looked up at you in fear as you order for her to be taken to your brothel. A hint of some emotion hidden in her eyes draws your attention for a moment before she unconsciously looked away, no doubt afraid of what you'd do to her if she met your gaze.";
			}
			else
			{
				/* */if (g_Dice % 2)	ss << "They were dragged crying and screaming to your current brothel afraid of what you might do to them as their new owner.";
				else/*            */	ss << "They looked up at you in fear as you order for them to be taken to your brothel. A hint of some emotion hidden in one of their eyes draws your attention for a moment before she unconsciously looked away, no doubt afraid of what you'd do to her if she met your gaze.";
			}
		}
		ss << "\n";
	}
#pragma endregion
#pragma region //	Send them to Your House		//
	else if  (m_TargetBuilding->type() == BuildingType::HOUSE)
	{
		if (g_Game.player().disposition() >= 80)				//Benevolent
		{
			ss << "";
		}
		else if (g_Game.player().disposition() >= 50)			// nice
		{
			ss << "";
		}
		else if (g_Game.player().disposition() >= 10)			//Pleasant
		{
			ss << "";
		}
		else if (g_Game.player().disposition() >= -10)			// neutral
		{
			ss << "";
		}
		else if (g_Game.player().disposition() >= -50)			// not nice
		{
			ss << "";
		}
		else if (g_Game.player().disposition() >= -80)			//Mean
		{
			ss << "";
		}
		else											//Evil
		{
			ss << "";
		}
	}
#pragma endregion
#pragma region //	Send them to the Clinic		//
	else if  (m_TargetBuilding->type() == BuildingType::CLINIC)
	{
		ss << (numgirls == 1 ? "She is" : "They are") << " brought to the Clinic where they are given a full checkup.\n";
		for (auto girl : girls_bought)
		{
			girl->health(100);
			girl->happiness(100);
			girl->tiredness(-100);
			girl->service(max(0, g_Dice.bell(-1, 3)));
			girl->medicine(max(0, g_Dice.bell(-2, 3)));
			girl->morality(max(0, g_Dice.bell(-2, 2)));
			if (girl->has_disease())
			{
				vector<string> diseases;
				if (girl->has_trait("AIDS"))		diseases.emplace_back("AIDS");
				if (girl->has_trait("Chlamydia"))	diseases.emplace_back("Chlamydia");
				if (girl->has_trait("Herpes"))		diseases.emplace_back("Herpes");
				if (girl->has_trait("Syphilis"))	diseases.emplace_back("Syphilis");
				ss << girl->m_Realname << " has been diagnosed with ";
				if (diseases.empty())	ss << "an unknown disease.";
				if (!diseases.empty())	ss << diseases[0];
				if (diseases.size() >= 2)	ss << " and " << diseases[1];
				if (diseases.size() >= 3)	ss << " and " << diseases[2];
				if (diseases.size() >= 4)	ss << " and " << diseases[3];
				ss << ".\n";
			}
		}
	}
#pragma endregion
#pragma region //	Send them to the Studio		//
	else if  (m_TargetBuilding->type() == BuildingType::STUDIO)
	{
		ss << "\"Ok " << (numgirls == 1 ? "my dear" : "ladies") << ", here are a couple of scripts to practice with on your way to the Studio.\"\n";
		if (numgirls == 1)		// single girl flavor texts
		{
            for (auto girl : girls_bought)
            {
				/* */if (girl->has_trait("Porn Star"))
				{
					ss << girl->m_Realname << ": \"Script? I don't need a script, just tell me who to fuck and where the camera is and I'll get it done.\"\n";
				}
				else if (girl->has_trait("Actress"))
				{
					ss << girl->m_Realname << ": \"This is a rather short script, I'm used to long dialogues. I don't seem to get a lot of lines here, how much camera time do I get?\"\nGuard: \"Don't worry sweetheart, your mouth will get a lot to do.\"\n";
				}
				else if (girl->has_trait("Shy"))
				{
					ss << girl->m_Realname << ": \"I can't act, I can barely talk to other people. I get tongue tied whenever I'm around people.\"\nGuard: \"Tongued? Tied? That shouldn't be a problem sweetheart, they can work with anything where you are going.\"\n";
					girl->happiness(-10);
				}
				else
				{
					ss << girl->m_Realname << " spends her time going over the scripts, disgusted at times, turned on at others.";
					girl->performance(g_Dice % 4);
					girl->libido(g_Dice % 10);
				}
			}
		}
		else					// multiple girl flavor texts
		{
			string pornstarname;	int pornstarnum = 0;
			string actressname;	int actressnum = 0;
			string shygirlname;	int shygirlnum = 0;

            for (auto girl : girls_bought)
            {
				/* */if (girl->has_trait("Porn Star"))	{ pornstarnum++; pornstarname = girl->m_Realname; }
				else if (girl->has_trait("Actress"))	{ actressnum++;  actressname = girl->m_Realname; }
				else if (girl->has_trait("Shy"))		{ shygirlnum++;  shygirlname = girl->m_Realname; }
				girl->lesbian(g_Dice % numgirls);
				girl->performance(g_Dice % numgirls);
			}
			if (actressnum > 0 && pornstarnum > 0)
			{
				ss << actressname << ": \"I've been in a few films already, this will be fun.\"\n" << pornstarname << ": \"Its not that kind of film honey.\"\n";
			}
			ss << "The girls practice their scripts, playing different roles and with eachother.\n";
		}
	}
#pragma endregion
#pragma region //	Send them to the Arena			//
	else if (m_TargetBuilding->type() == BuildingType::ARENA)
	{
		ss << "Guard: \"Ok " << (numgirls == 1 ? "sweetheart" : "ladies") << ", we are headed for the Arena where you will train and eventually fight for your new master, " << g_Game.player().RealName() << ". Your first lesson, to carry these heavy packages to the Arena. Load up and march.\"\n";
        for (auto girl : girls_bought)
        {
			girl->strength(g_Dice % 6);
			girl->constitution(g_Dice % 3);
			girl->tiredness(g_Dice % 30);
		}
	}
#pragma endregion
#pragma region //	Send them to the Centre		//
	else if (m_TargetBuilding->type() == BuildingType::CENTRE)
	{
		ss << "When " << (numgirls == 1 ? "she arrives" : "the girls arrive") << " at the Centre " << (numgirls == 1 ? "she is" : "they are") << " shown around and then assigned to " << (numgirls == 1 ? "clean the dishes.\n" : "various tasks around the Centre.\n");
        for (auto girl : girls_bought)
        {
			girl->service(g_Dice % 5);
			girl->morality(g_Dice % 5);
			girl->cooking(g_Dice % 3);
		}
	}
#pragma endregion
#pragma region //	Send them to the Farm			//
	else if (m_TargetBuilding->type() == BuildingType::FARM)
	{
		ss << (numgirls == 1 ? "She is" : "The girls are") << " brought to your stall at the town's farmers market until they are ready to go to the Farm. While there, " << (numgirls == 1 ? "she is" : "they are") << " shown the various animals and goods that they will be producing on the farm.\n";
        for (auto girl : girls_bought)
        {
			girl->farming(g_Dice % 5 + 1);
			girl->animalhandling(g_Dice % 5 + 1);
			girl->herbalism(g_Dice % 3);
		}
	}
#pragma endregion
	// `J` end flavor texts

	// `J` send them where they need to go
    for (auto girl : girls_bought)
    {
		stringstream sss;
		sss << "Purchased from the Slave Market for " << tariff.slave_buy_price(girl) << " gold.";
		girl->m_Events.AddMessage(sss.str(), IMGTYPE_PROFILE, EVENT_GOODNEWS);

		if(m_TargetBuilding) {
		    m_TargetBuilding->add_girl(girl);
            affect_girl_by_disposition(*girl);
		} else	// if something fails this will catch it and send them to the dungeon
		{
			g_Game.dungeon().AddGirl(girl, DUNGEON_NEWSLAVE);
			affect_dungeon_girl_by_disposition(*girl);
		}
		g_Game.RemoveMarketSlave(*girl);
	}

	// `J` now tell the player what happened
	if (numgirls <= 0)	g_Game.push_message("Error, no girls names in the list", 0);
	else g_Game.push_message(ss.str(), 0);

	// finish it
	m_SelectedGirl = -1;
	PrepareImage(image_id, nullptr, -1, false, -1, false, "blank");
	HideImage(image_id, true);		// hide/show image based on whether a girl is selected
	if (m_SelectedGirl < 0)								// if no girl is selected, clear girl info
	{
		EditTextItem("No girl selected", details_id);
		if (trait_id >= 0) EditTextItem("", trait_id);
	}

	return true;
}

void cScreenSlaveMarket::affect_girl_by_disposition(sGirl& girl) const
{
    if (g_Game.player().disposition() >= 80)				// Benevolent
    {
        girl.health(g_Dice % 10);
        girl.happiness(g_Dice % 20);
        girl.tiredness(-(g_Dice % 10));
        girl.pclove(g_Dice.bell(-1, 5));
        girl.pcfear(g_Dice.bell(-5, 1));
        girl.pchate(g_Dice.bell(-5, 1));
        girl.obedience(g_Dice.bell(-1, 5));
        girl.confidence(g_Dice.bell(-1, 5));
        girl.spirit(g_Dice.bell(-2, 10));
        girl.dignity(g_Dice.bell(-2, 5));
        girl.morality(g_Dice.bell(-2, 5));
        girl.refinement(g_Dice.bell(-2, 5));
        girl.sanity(g_Dice.bell(-1, 5));
        girl.fame(max(0, g_Dice.bell(-1, 1)));
    }
    else if (g_Game.player().disposition() >= 50)			// Nice
    {
        girl.health(g_Dice % 5);
        girl.happiness(g_Dice % 10);
        girl.tiredness(-(g_Dice % 5));
        girl.pclove(g_Dice.bell(-1, 3));
        girl.pcfear(g_Dice.bell(-3, 1));
        girl.pchate(g_Dice.bell(-3, 1));
        girl.obedience(g_Dice.bell(-1, 3));
        girl.confidence(g_Dice.bell(-1, 3));
        girl.spirit(g_Dice.bell(-1, 5));
        girl.dignity(g_Dice.bell(-1, 3));
        girl.morality(g_Dice.bell(-1, 3));
        girl.refinement(g_Dice.bell(-1, 3));
        girl.sanity(g_Dice.bell(-1, 3));
    }
    else if (g_Game.player().disposition() >= 10)			// Pleasant
    {
        girl.happiness(g_Dice % 5);
        girl.pclove(g_Dice.bell(-1, 1));
        girl.pcfear(g_Dice.bell(-1, 1));
        girl.pchate(g_Dice.bell(-1, 1));
        girl.obedience(g_Dice.bell(-1, 1));
        girl.confidence(g_Dice.bell(-1, 1));
        girl.spirit(g_Dice.bell(-1, 1));
        girl.dignity(g_Dice.bell(-1, 1));
        girl.morality(g_Dice.bell(-1, 1));
        girl.refinement(g_Dice.bell(-1, 1));
        girl.sanity(g_Dice.bell(-1, 1));
    }
    else if (g_Game.player().disposition() >= -10)			// Neutral
    {
    }
    else if (g_Game.player().disposition() >= -50)			// Not nice
    {
        girl.health(-(g_Dice % 2));
        girl.happiness(-(g_Dice % 10));
        girl.tiredness(-(g_Dice % 3));
        girl.pclove(-(g_Dice % 5));
        girl.pcfear(g_Dice % 5);
        girl.pchate(g_Dice % 5);
        girl.obedience(g_Dice.bell(-1, 2));
        girl.confidence(-(g_Dice % 3));
        girl.spirit(-(g_Dice % 5));
        girl.dignity(-(g_Dice % 3));
        girl.morality(-(g_Dice % 2));
        girl.refinement(-(g_Dice % 2));
        girl.sanity(g_Dice.bell(-2, 2));
        girl.bdsm(max(0, g_Dice.bell(-2, 5)));
    }
    else if (g_Game.player().disposition() >= -80)			//Mean
    {
        girl.health(-(g_Dice % 3));
        girl.happiness(-(g_Dice % 20));
        girl.tiredness(-(g_Dice % 5));
        girl.pclove(-(g_Dice % 10));
        girl.pcfear(g_Dice % 10);
        girl.pchate(g_Dice % 10);
        girl.obedience(g_Dice.bell(-1, 4));
        girl.confidence(-(g_Dice % 5));
        girl.spirit(-(g_Dice % 10));
        girl.dignity(-(g_Dice % 10));
        girl.morality(-(g_Dice % 2));
        girl.refinement(-(g_Dice % 3));
        girl.sanity(g_Dice.bell(-3, 1));
        girl.bdsm(3 + g_Dice % 10);
    }
    else											// Evil
    {
        girl.health(-(g_Dice % 5));
        girl.happiness(-(g_Dice % 40));
        girl.tiredness(-(g_Dice % 10));
        girl.pclove(-(g_Dice % 20));
        girl.pcfear(g_Dice % 20);
        girl.pchate(g_Dice % 20);
        girl.obedience(g_Dice.bell(-2, 5));
        girl.confidence(-(g_Dice % 10));
        girl.spirit(-(g_Dice % 20));
        girl.dignity(-(g_Dice % 20));
        girl.morality(-(g_Dice % 3));
        girl.refinement(-(g_Dice % 5));
        girl.sanity(g_Dice.bell(-5, 1));
        girl.bdsm(5 + g_Dice % 12);
    }
}

bool cScreenSlaveMarket::change_selected_girl(int selected)
{
	ImageNum       = -1;
	/*
	 *	Since this is a multiselect box, GetLastSelectedItemFromList
	 *	returns the last clicked list item, even if it's deselected.
	 *	So, we'll check for that and show first truly selected item
	 *	if the last clicked one is actually deselected.
	 */
	m_SelectedGirl = selected;
	if (m_SelectedGirl < 0)
	{
		HideImage(image_id, (m_SelectedGirl < 0));		// hide/show image based on whether a girl is selected
		if (m_SelectedGirl < 0)								// if no girl is selected, clear girl info
		{
			EditTextItem("No girl selected", details_id);
			if (trait_id >= 0) EditTextItem("", trait_id);
		}
	}
	bool MatchSel = false;
	int i;

	for (i = multi_slave_first(); i != -1; i = multi_slave_next())
	{
		if (i == m_SelectedGirl)
		{
			MatchSel = true;
			break;
		}
	}
	if (!MatchSel) m_SelectedGirl = multi_slave_first();
	// if the player selected an empty slot make that into "nothing selected" and return
	//if (MarketSlaveGirls[selection] == nullptr) selection = -1;
	// disable/enable buttons based on whether a girl is selected
	DisableButton(more_id, (m_SelectedGirl == -1));
	DisableButton(buy_slave_id, (m_SelectedGirl == -1));
	if (trait_list_id >= 0) ClearListBox(trait_list_id);
	if (trait_list_text_id >= 0) EditTextItem("Traits:", trait_list_text_id);
	if (girl_desc_id >= 0)	EditTextItem("", girl_desc_id);
	// selection of -1 means nothing selected so we get to go home early
	if (m_SelectedGirl == -1) return true;
	/*
	 *	otherwise, we have (potentially) a new girl:
	 *	set the global girl pointer
	 *
	 *	if we can't find the pointer. log an error and go home
	 */
	sGirl *girl = g_Game.GetMarketSlave(m_SelectedGirl);
	if (!girl)
	{
		g_LogFile.ss() << "Warning: cScreenSlaveMarket::change_selected_girl" << "can't find girl data for selection";
		g_LogFile.ssend();
		return true;
	}
	string detail;

	if (DetailLevel == 0)		detail = cGirls::GetDetailsString(girl, true);
	else if (DetailLevel == 1)	detail = cGirls::GetMoreDetailsString(girl, true);
	else						detail = cGirls::GetThirdDetailsString(girl);
	EditTextItem(detail, details_id);
	ImageNum = -1;										// I don't understand where this is used...

	preparescreenitems(girl);
	PrepareImage(image_id, girl, IMGTYPE_PRESENTED, true, ImageNum);

	return true;
}

string cScreenSlaveMarket::get_buy_slave_string(sGirl* girl)
{
	string text = girl->m_Realname;

	if (active_building().free_rooms() <= 0)
	{
		text += " has been sent to your dungeon, since your current brothel is full.";
	}
	else if (cGirls::GetRebelValue(girl, false) >= 35)
	{
		if (g_Game.player().disposition() >= 80)				//Benevolent
		{
			text += " accepting her own flaws that to needed be corrected, she goes to the dungeon where she will be waiting for your guidance.";
		}
		else if (g_Game.player().disposition() >= 50)			// nice
		{
			text += " in your opinion needs to work on her attitude, she has been guided to the dungeon.";
		}
		else if (g_Game.player().disposition() >= 10)			//Pleasant
		{
			text += " as your newest investment, she was sent to the dungeon to work on her rebellious nature.";
		}
		else if (g_Game.player().disposition() >= -10)			// neutral
		{
			text += " has been sent to your dungeon, as she is rebellious and poorly trained.";
		}
		else if (g_Game.player().disposition() >= -50)			// not nice
		{
			text += " as your newest investment that needs your special touch, she was sent to the dungeon.";
		}
		else if (g_Game.player().disposition() >= -80)			//Mean
		{
			text += " still had some spirit in her eyes left that you decided to stub out. She was dragged to a dungeon cell.";
		}
		else											//Evil
		{
			text += " put up a fight. She was beaten and dragged to your dungeon where you can have some private fun time with her.";
		}
	}
	else
	{
		int t = g_Dice % 2;	// `J` because there are currently only 2 text options per disposition this should be ok
		if (g_Game.player().disposition() >= 80)				//Benevolent
		{
			if (t == 1)	text += " went to your current brothel with a smile on her face happy that such a nice guy bought her.";
			else		text += " smiled as you offered her your arm, surprised to find such a kindness waiting for her. Hopeing such kindness would continue, she went happily with you as her owner.";
		}
		else if (g_Game.player().disposition() >= 50)			// nice
		{
			if (t == 1)	text += ", having heard about her new owner's reputation, was guided to your current brothel without giving any trouble.";
			else		text += " looked up at you hopefully as you refused the use of a retainer or delivery, instead finding herself taken into your retinue for the day and given a chance to enjoy the fresh air before you both return home.";
		}
		else if (g_Game.player().disposition() >= 10)			//Pleasant
		{
			if (t == 1)	text += " was sent to your current brothel, knowing that she could have been bought by a lot worse owner.";
			else		text += " was escorted home by one of your slaves who helped her settle in. She seems rather hopeful of a good life in your care.";
		}
		else if (g_Game.player().disposition() >= -10)			// neutral
		{
			if (t == 1)	text += " as your newest investment, she was sent to your current brothel.";
			else		text += " has been sent to your establishment under the supervision of your most trusted slaves.";
		}
		else if (g_Game.player().disposition() >= -50)			// not nice
		{
			if (t == 1)	text += " not being very happy about her new owner, was escorted to your current brothel.";
			else		text += " struggled as her hands were shackled in front of her. Her eyes locked on the floor, tears gathering in the corners of her eyes, as she was sent off to your brothel.";
		}
		else if (g_Game.player().disposition() >= -80)			//Mean
		{
			if (t == 1)	text += " didn't wanted to provoke you in any possible way. She went quietly to your current brothel, without any resistance.";
			else		text += " was dragged away to your brothel crying, one of your guards slapping her face as she tried to resist. ";
		}
		else											//Evil
		{
			if (t == 1)	text += " was dragged crying and screaming to your current brothel afraid of what you might do to her as her new owner.";
			else		text += " looked up at you in fear as you order for her to be taken to your brothel. A hint of some emotion hidden in her eyes draws your attention for a moment before she unconsciously looked away, no doubt afraid of what you'd do to her if she met your gaze.";
		}
	}
	return text;
}

void cScreenSlaveMarket::preparescreenitems(sGirl* girl)
{

	int trait_count = 0;
	stringstream traits_text;
	traits_text << "Traits:      ";

	// loop through her traits, populating the box
	for (int i = 0; i < MAXNUM_TRAITS; i++)
	{
		if (!girl->m_Traits[i]) continue;
		trait_count++;
		if (trait_list_id >= 0) AddToListBox(trait_list_id, i, girl->m_Traits[i]->display_name());
		if (trait_list_text_id)
		{
			if (trait_count > 1) traits_text << ",   ";
			traits_text << girl->m_Traits[i]->display_name();
		}
	}
	// and finally, highlight the selected entry?
	if (trait_list_id >= 0) SetSelectedItemInList(trait_list_id, 0);
	if (trait_list_text_id >= 0) EditTextItem(traits_text.str(), trait_list_text_id);
	if (girl_desc_id >= 0)	EditTextItem(girl->m_Desc, girl_desc_id);
}

void cScreenSlaveMarket::affect_dungeon_girl_by_disposition(sGirl& girl) const
{
    if (g_Game.player().disposition() >= 80)                //Benevolent
    {
            // you have a reputation for not torturing so they are not afraid (but you are sending them to a dungeon so...)
            girl.m_AccLevel = 0;
            girl.pcfear(g_Dice.bell(-3, 1));
            girl.pchate(g_Dice.bell(-3, 1));
            girl.pclove(g_Dice.bell(-1, 3));
            girl.happiness(g_Dice.bell(-5, 10));
            girl.spirit(g_Dice.bell(-2, 4));
            girl.obedience(g_Dice.bell(-2, 5));

    } else if (g_Game.player().disposition() >= 50)            // nice
    {
        // you have a reputation for not torturing much so they are less afraid (but you are sending them to a dungeon so...)
        girl.m_AccLevel = 0;
        girl.pcfear(g_Dice.bell(-2, 2));
        girl.pchate(g_Dice.bell(-2, 2));
        girl.pclove(g_Dice.bell(-2, 2));
        girl.happiness(g_Dice.bell(-10, 5));
        girl.spirit(g_Dice.bell(-2, 2));
        girl.obedience(g_Dice.bell(-2, 2));

    } else if (g_Game.player().disposition() >= 10)            //Pleasant
    {
        bool mas = girl.has_trait("Masochist");
        girl.m_AccLevel = 0;
        girl.pcfear(g_Dice % 5 + (mas ? 0 : 3));
        girl.pchate(g_Dice % 3 + (mas ? 0 : 2));
        girl.pclove(-(g_Dice % 6 - (mas ? 2 : 0)));
        girl.happiness(-(g_Dice % 6 - (mas ? 2 : 0)));
        girl.spirit(-(g_Dice % 4 - (mas ? 2 : 0)));
        girl.obedience(g_Dice % 3 - 1);
    } else if (g_Game.player().disposition() >= -10)            // neutral
    {
        bool mas = girl.has_trait("Masochist");
        girl.m_AccLevel = 0;
        girl.pcfear(g_Dice % 5 + (mas ? 0 : 3));
        girl.pchate(g_Dice % 3 + (mas ? 0 : 2));
        girl.pclove(-(g_Dice % 6 - (mas ? 2 : 0)));
        girl.happiness(-(g_Dice % 6 - (mas ? 2 : 0)));
        girl.spirit(-(g_Dice % 4 - (mas ? 2 : 0)));
        girl.obedience(g_Dice % 3 - 1);
    } else if (g_Game.player().disposition() >= -50)            // not nice
    {
        bool mas = girl.has_trait("Masochist");
        girl.m_AccLevel = 0;
        girl.pcfear(g_Dice % 10 + (mas ? 0 : 5));
        girl.pchate(g_Dice % 5 + (mas ? 0 : 3));
        girl.pclove(-(g_Dice % 12 - (mas ? 3 : 0)));
        girl.happiness(-(g_Dice % 12 - (mas ? 3 : 0)));
        girl.spirit(-(g_Dice % 7 - (mas ? 2 : 0)));
        girl.obedience(g_Dice % 5 - 2);

    } else if (g_Game.player().disposition() >= -80)            //Mean
    {

        bool mas = girl.has_trait("Masochist");
        girl.m_AccLevel = 0;
        girl.pcfear(g_Dice % 20 + (mas ? 0 : 10));
        girl.pchate(g_Dice % 10 + (mas ? 0 : 5));
        girl.pclove(-(g_Dice % 25 - (mas ? 5 : 0)));
        girl.happiness(-(g_Dice % 25 - (mas ? 5 : 0)));
        girl.spirit(-(g_Dice % 15 - (mas ? 3 : 0)));
        girl.obedience(g_Dice % 10 - 3);

    } else/*                                        */    //Evil
    {
        bool mas = girl.has_trait("Masochist");
        girl.m_AccLevel = 0;
        girl.pcfear(g_Dice % 40 + (mas ? 0 : 20));
        girl.pchate(g_Dice % 20 + (mas ? 0 : 10));
        girl.pclove(-(g_Dice % 50 - (mas ? 10 : 0)));
        girl.happiness(-(g_Dice % 50 - (mas ? 10 : 0)));
        girl.spirit(-(g_Dice % 30 - (mas ? 5 : 0)));
        girl.obedience(g_Dice % 20 - 5);
    }
}

void cScreenSlaveMarket::change_release(BuildingType target, int index)
{
    stringstream ss;
    m_TargetBuilding = g_Game.buildings().building_with_type(target, index);

    ss.str("");
    ss << "Send Girl to: " << m_TargetBuilding->name();
    EditTextItem(ss.str(), releaseto_id);
    ss.str("");
    ss << "Room for " << m_TargetBuilding->free_rooms() << " more girls.";
    EditTextItem(ss.str(), roomsfree_id);
}
