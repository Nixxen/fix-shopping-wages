#include <Debug.h>

#include <core/Functions.h>

#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/InputHandler.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObjectBase.h>
#include <kenshi/util/TimeOfDay.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

// -----------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------

// TODO Make configuration configurable through Emkej's Mod Core

// Price of one Dried Meat per day. Resolved at runtime from an NPC's inventory.
//  Initialized to -1 (sentinel); resolved on first dailyUpdate that finds Dried Meat.
static int kUniversalWage = -1;
// Maximum number of days' wages that an NPC can accumulate without spending any money.
static const int kMaxSavingsMultiplier = 10;

// -----------------------------------------------------------------------
// Helper: look up an integer value from GameData::idata by key
// -----------------------------------------------------------------------
static int LookupGameDataInteger(GameData *gameData, const std::string &key, int defaultValue)
{
    if (gameData == nullptr) { return defaultValue; }

    typedef boost::unordered::unordered_map<
        std::string, int, boost::hash<std::string>, std::equal_to<std::string>,
        Ogre::STLAllocator<std::pair<const std::string, int>, Ogre::GeneralAllocPolicy>>
        IntegerDataMap;

    IntegerDataMap &integerMap = gameData->idata;
    IntegerDataMap::const_iterator iterator = integerMap.find(key);
    if (iterator != integerMap.end()) { return iterator->second; }
    return defaultValue;
}

// -----------------------------------------------------------------------
// Debug diagnostics (inlined from FixShoppingWagesDebug.inl)
// -----------------------------------------------------------------------
#include "FixShoppingWagesDebug.inl"

// -----------------------------------------------------------------------
// Resolve kUniversalWage at runtime by scanning NPC inventories for Dried Meat
// -----------------------------------------------------------------------
static int ResolveUniversalWage()
{
    typedef ogre_unordered_set<Character *>::type CharacterSet;
    const CharacterSet &characterList = ou->getCharacterUpdateList();

    for (CharacterSet::const_iterator iterator = characterList.begin(); iterator != characterList.end(); ++iterator)
    {
        Character *character = *iterator;
        Inventory *inventory = character->getInventory();
        if (inventory == nullptr) { continue; }

        lektor<Item *> foodItems;
        inventory->getAllItemsWithFunction(foodItems, ITEM_FOOD);

        int itemCount = static_cast<int>(foodItems.size());
        for (int index = 0; index < itemCount; ++index)
        {
            if (foodItems[index]->getName() == "Dried Meat")
            {
                int price = foodItems[index]->getAvgPrice();
                std::stringstream message;
                message << "Resolved Dried Meat price from NPC inventory: " << price;
                DebugLog(message.str());
                return price;
            }
        }
    }
    return -1;
}

// -----------------------------------------------------------------------
// Hook: GameWorld::dailyUpdates. Give NPCs their daily wage allowance
// -----------------------------------------------------------------------
static void (*GameWorld_dailyUpdates_originalFunction)(GameWorld *) = nullptr;

void GameWorld_dailyUpdates_hook(GameWorld *thisWorld)
{
    if (kVerboseDebugLogging) { LogDailyUpdatesStart(thisWorld); }

    GameWorld_dailyUpdates_originalFunction(thisWorld);

    // Resolve universal wage on first daily update that finds Dried Meat
    const int driedMeatDefaultPrice = 78;
    if (kUniversalWage == -1) { kUniversalWage = ResolveUniversalWage(); }

    typedef ogre_unordered_set<Character *>::type CharacterSet;
    const CharacterSet &characterList = ou->getCharacterUpdateList();

    int nonPlayerCharacterCount = 0;
    int changedCharacterCount = 0;
    int fallbackWage = (kUniversalWage != -1) ? kUniversalWage : driedMeatDefaultPrice;

    for (CharacterSet::const_iterator iterator = characterList.begin(); iterator != characterList.end(); ++iterator)
    {
        Character *character = *iterator;
        if (character->isPlayerCharacter()) { continue; }

        DailyUpdateSnapshot beforeSnapshot;
        if (kVerboseDebugLogging) { CaptureSnapshot(character, beforeSnapshot); }

        int wages = LookupGameDataInteger(character->data, "wages", 0);

        if (wages == 0)
        {
            int minMoney = LookupGameDataInteger(character->data, "money min", 0);
            int maxMoney = LookupGameDataInteger(character->data, "money max", 0);
            if (maxMoney > minMoney) { wages = minMoney + (std::rand() % (maxMoney - minMoney + 1)); }
            if (wages == 0) { wages = fallbackWage; }
        }

        int currentMoney = character->getMoney();
        // NOTE: For characters without a fixed wage, this may fluctuate between days. You win more some days.
        int maxSavings = wages * kMaxSavingsMultiplier;
        int newMoney = (std::min)(currentMoney + wages, maxSavings);

        int moneyToGive = newMoney - currentMoney;
        if (moneyToGive > 0) { character->takeMoney(-moneyToGive); }

        if (kVerboseDebugLogging) { LogDailyUpdateCharacterDiff(character, beforeSnapshot, changedCharacterCount); }

        ++nonPlayerCharacterCount;
    }

    if (kVerboseDebugLogging) { LogDailyUpdatesEnd(nonPlayerCharacterCount, changedCharacterCount, thisWorld); }
}

// -----------------------------------------------------------------------
// Hook: PlayerInterface::updateUT
// -----------------------------------------------------------------------
static void (*PlayerInterface_updateUT_originalFunction)(PlayerInterface *) = nullptr;

void PlayerInterface_updateUT_hook(PlayerInterface *thisPointer)
{
    PlayerInterface_updateUT_originalFunction(thisPointer);

    if (kVerboseDebugLogging)
    {
        LogDayTransition();
        LogHourlySnapshotDiff();
    }
    if (kDeveloperDebug) { LogHotkeys(); }
}

// -----------------------------------------------------------------------
// Plugin entry point
// -----------------------------------------------------------------------
__declspec(dllexport) void startPlugin()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
                                  KenshiLib::GetRealAddress(&PlayerInterface::updateUT), PlayerInterface_updateUT_hook,
                                  &PlayerInterface_updateUT_originalFunction
                              ))
    {
        ErrorLog("Could not hook PlayerInterface::updateUT");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
                                  KenshiLib::GetRealAddress(&GameWorld::dailyUpdates), GameWorld_dailyUpdates_hook,
                                  &GameWorld_dailyUpdates_originalFunction
                              ))
    {
        ErrorLog("Could not hook GameWorld::dailyUpdates");
        return;
    }
}