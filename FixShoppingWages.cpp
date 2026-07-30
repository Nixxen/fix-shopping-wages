#include <Debug.h>

#include <core/Functions.h>

#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/InputHandler.h>
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

// Price of one Dried Meat per day. Fallback when no wages/min/max are defined
// TODO: Read from GameData instead of hardcoding. This is my modded price.
static const int kUniversalWage = 106;
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
// Hook: GameWorld::dailyUpdates. Give NPCs their daily wage allowance
// -----------------------------------------------------------------------
static void (*GameWorld_dailyUpdates_originalFunction)(GameWorld *) = nullptr;

void GameWorld_dailyUpdates_hook(GameWorld *thisWorld)
{
    if (kVerboseDebugLogging) { LogDailyUpdatesStart(thisWorld); }

    GameWorld_dailyUpdates_originalFunction(thisWorld);

    typedef ogre_unordered_set<Character *>::type CharacterSet;
    const CharacterSet &characterList = ou->getCharacterUpdateList();

    int nonPlayerCharacterCount = 0;
    int changedCharacterCount = 0;

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
            if (wages == 0) { wages = kUniversalWage; }
        }

        int currentMoney = character->getMoney();
        // NOTE: Due to the random min/max wages, this may fluctuate between days. You win more some days.
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