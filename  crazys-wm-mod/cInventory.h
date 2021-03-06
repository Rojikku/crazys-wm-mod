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
#ifndef __CINVENTORY_H
#define __CINVENTORY_H

// includes
#include <ostream>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "Constants.h"
#include <memory>

using namespace std;

struct sGirl;
class TiXmlElement;

struct sCondition {
public:
    sCondition();
    bool check(const sGirl& girl) const;

    void from_xml(TiXmlElement& element);
private:
    struct sSource {
        virtual ~sSource() = default;
        virtual int get(const sGirl& girl) = 0;
    };

    struct sSkillSource : sSource {
        SKILLS skill;
        int get(const sGirl& girl) override;
    };

    struct sStatSource : sSource {
        STATS stat;
        int get(const sGirl& girl) override;
    };

    std::unique_ptr<sSource> m_ValueSource;
    int m_ReferenceValue;

    enum class Cmp {
        GREATER, LOWER
    } m_Comparison;
};

struct sEffect
{
	// MOD docclox
/*
 *	let's have an enum for possible values of m_Affects
 */
	enum What {
		Skill		= 0,
		Stat		= 1,
		Nothing		= 2,
		GirlStatus	= 3,
		Trait		= 4,
		Enjoy		= 5
	};
	What m_Affects;
/*
 *	define an ostream operator so it will pretty print
 *	(more useful for debugging than game play
 *	but why take it out?)
 */
	friend ostream& operator << (ostream& os, sEffect::What &w);
/*
 *	and a function to go the other way
 *	we need this to turn the strings in the xml file
 *	into numbers
 */
	void set_what(string s);
/*
 *	can't make an enum for this since it can represent
 *	different quantites.
 *
 *	The OO approach would be to write some variant classes, I expect
 *	but really? Life's too short...
 */
	unsigned char m_EffectID;	// what stat, skill or status effect it affects
/*
 *	but I still need strings for the skills, states, traits and so forth
 *
 *	these should be (were until the merge) in sGirl. Will be again
 *	as soon as I sort the main mess out...
 */
	const char *girl_status_name(unsigned int id);
	const char *skill_name(unsigned int id);		// WD:	Use definition in sGirl::
	const char *stat_name(unsigned int id);			// WD:	Use definition in sGirl::
	const char *enjoy_name(unsigned int id);		// `J`	Use definition in sGirl::

/*
 *	and we need to go the other way,
 *	setting m_EffectID from the strings in the XML file
 *
 *	WD:	Change to use definition and code in sGirl::
 *		remove duplicated code
 */
	bool set_skill(string s);
	bool set_girl_status(string s);
	bool set_stat(string s);
	bool set_Enjoyment(string s);


/*
 *	magnitude of the effect. 
 *	-10 will subtract 10 from the target stat while equiped
 *	and add 10 when unequiped.
 *
 *	With status effects and traits 1 means add,
 *	0 means take away and 2 means disable
 */
	int m_Amount;
	
	int m_Duration;	// `J` added for temporary trait duration
/*
 *	name of the trait it adds
 */
	string m_Trait;

	// chance that the effect happens (percent)
	// this only makes sense for consumable items, otherwise just re-equipping
	// can change this effect.
	int m_Chance = 100;

/*
 *	and a pretty printer for the class as a whole
 *	just a debug thing, really
 */
	friend ostream& operator << (ostream& os, sEffect &eff);
	// end mod
};

class TiXmlElement;

class CraftingData {
public:
    /// checks whether the given girl can craft the item with the given job and craft points
    bool can_craft(sGirl& girl, JOBS job, int craft_points) const;

    /// checks whether the item is in principle craftable by the given job
    bool is_craftable_by(JOBS job) const;

    void from_xml(TiXmlElement& element);

    int craft_cost() const;
    int mana_cost() const;
    int weight() const;
private:
    /// set of jobs that can craft this item
    std::set<JOBS> m_CraftableBy;

    /// creation cost
    int m_CraftPoints = 0;
    /// mana cost
    int m_ManaCost    = 0;
    /// relative weight for selecting this item when choosing what to craft
    int m_Weight      = 1;

    /// TODO skill, attribute, level conditions
    std::map<SKILLS, int> m_SkillRequirements;
    std::map<STATS, int> m_StatsRequirements;
};


struct sInventoryItem
{
	string m_Name;
	string m_Desc;
	/*
	 *	item type: let's make an enum
	 */
	enum Type {
		Ring = 1,			// worn on fingers (max 8)
		Dress = 2,			// Worn on body, (max 1)
		Shoes = 3,			// worn on feet, (max 1)
		Food = 4,			// Eaten, single use
		Necklace = 5,		// worn on neck, (max 1)
		Weapon = 6,			// equiped on body, (max 2)
		Makeup = 7,			// worn on face, single use
		Armor = 8,			// worn on body over dresses (max 1)
		Misc = 9,			// random stuff. may cause a constant effect without having to be equiped
		Armband = 10,		// (max 2), worn around arms
		SmWeapon = 11,		// (max 2), hidden on body
		Underwear = 12,		// (max 1) worn under dress
		Hat = 13,			// CRAZY added this - Noncombat worn on the head (max 1)
		Helmet = 14,		// CRAZY added this	- Combat worn on the head (max 1)
		Glasses = 15,		// CRAZY added this	- Glasses (max 1)
		Swimsuit = 16,		// CRAZY added this - Swimsuit (max 1 in use but can have as many as they want)
		Combatshoes = 17,	// `J`   added this - Combat Shoes (max 1) often unequipped outside of combat
		Shield = 18			// `J`   added this - Shields (max 1) often unequipped outside of combat
	};
	Type m_Type;
	/*
	 *	and another for special values
	 */
	enum Special {
		None = 0,
		AffectsAll = 1,
		Temporary = 2
	};
	Special m_Special;
	/*
	 *	if 1 then this item doesn't run out if stocked in shop inventory
	 */
	bool m_Infinite;
	/*
	 *	the number of effects this item has
	 */
	vector<sEffect> m_Effects;
	/*
	 *	how much the item is worth?
	 */
	long m_Cost;
	/*	0 is good, while badness > is evil.
	 *	Girls may fight back if badness > 0,
	 *	Girls won't normally buy items > 20 on their own
	 *      default formula is -5% chance to buy on their own per Badness point (5 Badness = 75% chance)
	 *	Girls with low obedience may take off something that is bad for them
	 */
	int m_Badness;
	int m_GirlBuyChance;  // chance that a girl on break will buy this item if she's looking at it in the shop
	/*
	 *	0 means common,
	 *	1 means 50% chance of appearing in shops,
	 *	2 means 25% chance,
	 *	3 means 5% chance and
	 *	4 means only 15% chance of being found in catacombs,
	 *	5 means ONLY given in scripts and
	 *	6 means same as 5 but also may be given as a reward for objective
	 7 means only 5% chance in catacombs (catacombs05)
	 8 means only 1% chance in catacombs (catacombs01)
	 */
	enum Rarity {
		Common = RARITYCOMMON,			// old 0
		Shop50 = RARITYSHOP50,			// old 1
		Shop25 = RARITYSHOP25,			// old 2
		Shop05 = RARITYSHOP05,			// old 3
		Catacomb15 = RARITYCATACOMB15,		// old 4
		Catacomb05 = RARITYCATACOMB05,		// old 7  // MYR: Added 05 and 01 for the really, really valuable things like invulnerability
		Catacomb01 = RARITYCATACOMB01,		// old 8
		ScriptOnly = RARITYSCRIPTONLY,		// old 5
		ScriptOrReward = RARITYSCRIPTORREWARD	// old 6
	};
	Rarity m_Rarity;

	void set_rarity(const string& s);
	void set_special(const string& s);
	void set_type(const string& s);

	CraftingData m_Crafting;

	friend ostream& operator << (ostream& os, sInventoryItem::Special &spec);
	friend ostream& operator << (ostream& os, sInventoryItem::Rarity &r);
	friend ostream& operator << (ostream& os, sInventoryItem::Type &typ);
	friend ostream& operator << (ostream& os, sInventoryItem &it);
};

int part(int first,int last,sInventoryItem * curr[]);

class cInventory
{
public:
	cInventory() {
		for(auto & m_ShopItem : m_ShopItems) {
			m_ShopItem = nullptr;
		}
		m_NumShopItems = 0;
	}
	~cInventory();

	void Free();

    bool LoadItemsXML(const string& filename);
	void UpdateShop();	// re-randomizes the shops inventory
	sInventoryItem* GetItem(string name);
	sInventoryItem* GetShopItem(int num);
	int GetRandomShopItem();
	sInventoryItem* GetRandomItem();
	sInventoryItem* GetRandomCatacombItem();

	sInventoryItem* GetCraftableItem(sGirl& girl, JOBS job, int craft_points);
	/// return all items that can be crafted by a given job.
	std::vector<sInventoryItem*> GetCraftableItems(JOBS job);

	int CheckShopItem(string name);	// checks if a item is in shop inventory, returns -1 if not and the id if it is
	sInventoryItem* BuyShopItem(int num);	// removes and returns the item from the shop
	int BuyShopItem(sInventoryItem* item, int amount);
	bool GirlBuyItem(sGirl* girl, int ShopItem, int MaxItems, bool AutoEquip);  // girl buys selected item if possible; returns true if bought

	void Equip(sGirl* girl, int num, bool force);
	void Equip(sGirl* girl, sInventoryItem* item, bool force);
	void Unequip(sGirl* girl, int num);

	void AddItem(sInventoryItem* item);
	void CalculateCost(sInventoryItem* newItem);

	int HappinessFromItem(const sInventoryItem * item);  // determines how happy a girl will be to receive an item (or how unhappy to lose it)

	void GivePlayerAllItems();

	bool IsItemEquipable(sInventoryItem* item)
	{
        return item->m_Type != INVMISC;
    }

	bool	equip_limited_item_ok(sGirl*, int, bool, int);
	bool	equip_pair_ok(sGirl*, int, bool);
	bool	equip_ring_ok(sGirl*, int, bool);
	bool	equip_singleton_ok(sGirl*, int, bool);
	bool	ok_2_equip(sGirl*, int, bool);
	void	remove_trait(sGirl*, int, int);


private:
	std::vector<sInventoryItem *> m_Items;  // Master list of items?
	int m_NumShopItems;	// number of items in the shop
	sInventoryItem* m_ShopItems[NUM_SHOPITEMS];	// pointers to all items, the shop can only hold 30 random items

	//
};

#endif
