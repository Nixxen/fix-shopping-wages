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
#include <kenshi/RootObjectFactory.h>
#include <kenshi/util/TimeOfDay.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

// -----------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------

#include "FixShoppingWagesSharedContracts.h"

static const char *kPluginName = "Fix Shopping Wages";
static const char *kConfigFileName = "mod-config.json";
static const int kMaxBaseWageFallback = 100000;
static const int kMaxMaxSavingsMultiplier = 1000;
static const int kMaxBaseWageOverrideValue = 100000;

static PluginConfig gConfig = {
    true,  // enabled
    true,  // verboseDebugLogging
    true,  // limitVerboseDebugLogging
    true,  // developerDebug
    78,    // baseWageFallback
    false, // baseWageOverride
    78,    // baseWageOverrideValue
    10     // maxSavingsMultiplier
};

static std::string gSettingsPath;
static bool gConfigNeedsWriteBack = false;

// -----------------------------------------------------------------------
// Config state management (needed before parser include for forward refs)
// -----------------------------------------------------------------------
static std::string TrimAscii(const std::string &value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
    {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return value.substr(start, end - start);
}

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
// Resolve universal wage by spawning a Dried Meat instance and reading its price
// -----------------------------------------------------------------------
static int ResolveUniversalWage()
{
    GameData *itemData = ou->gamedata.getDataByName("Dried Meat", ITEM);
    if (itemData == nullptr)
    {
        DebugLog("ResolveUniversalWage: could not find Dried Meat GameData");
        return -1;
    }

    Item *item = ou->theFactory->createItem(
        itemData,
        hand(),  // null handle
        nullptr, // no weapon mesh override
        nullptr, // default material
        -1,      // Assumption: default level of gear
        nullptr  // no faction uniform
    );
    if (item == nullptr)
    {
        DebugLog("createItem for Dried Meat failed; cannot resolve universal wage");
        return -1;
    }

    int price = item->getAvgPrice();
    ou->destroy(item, false, "debug query");

    std::stringstream message;
    message << "Resolved Universal Wage from Dried Meat instance: " << price;
    DebugLog(message.str());
    return price;
}

// -----------------------------------------------------------------------
// Give an NPC their daily wage allowance
// -----------------------------------------------------------------------
static void GiveDailyWage(
    Character *character, int fallbackWage, int &changedCharacterCount, const DailyUpdateSnapshot &beforeSnapshot
)
{
    int wages = LookupGameDataInteger(character->data, "wages", -1);

    if (wages == -1)
    {
        // The character either has a sentinel value of -1 for wages, or the "wages" key is missing from their
        // GameData entirely. In either case, we do not give them any money.
        return;
    }

    if (wages == 0)
    {
        int maxMoney = LookupGameDataInteger(character->data, "money max", -1);
        if (maxMoney == -1)
        {
            // The character either has a sentinel value of -1 for money max, or the "money max" key is missing from
            // their GameData entirely. In either case, we do not give them any money.
            return;
        }

        int minMoney = LookupGameDataInteger(character->data, "money min", 0);
        if (maxMoney > minMoney) { wages = minMoney + (std::rand() % (maxMoney - minMoney + 1)); }
        if (wages == 0) { wages = fallbackWage; }
    }

    int currentMoney = character->getMoney();
    // NOTE: For characters without a fixed wage, this may fluctuate between days due to random variations.
    //  Depending on how far from the maximum they are they could get a lower cap one day and not get money, but the
    //  next day they could get a higher cap and get money.
    int maxSavings = wages * gConfig.maxSavingsMultiplier;
    int newMoney = (std::min)(currentMoney + wages, maxSavings);

    int moneyToGive = newMoney - currentMoney;
    if (moneyToGive > 0) { character->takeMoney(-moneyToGive); }

    if (gConfig.verboseDebugLogging) { LogDailyUpdateCharacterDiff(character, beforeSnapshot, changedCharacterCount); }
}

// -----------------------------------------------------------------------
// Config parsing and state helpers (inlined)
// -----------------------------------------------------------------------
#include "FixShoppingWagesConfigParsing.inl"

static void LoadConfigState()
{
    gConfigNeedsWriteBack = false;
    gConfig.enabled = true;
    gConfig.verboseDebugLogging = false;
    gConfig.limitVerboseDebugLogging = false;
    gConfig.developerDebug = false;
    gConfig.baseWageFallback = 78;
    gConfig.baseWageOverride = false;
    gConfig.baseWageOverrideValue = 78;
    gConfig.maxSavingsMultiplier = 10;

    if (gSettingsPath.empty()) { return; }

    bool foundConfigFile = false;
    bool needsWriteBack = false;
    if (!ReadConfigFromFile(gSettingsPath, &gConfig, &foundConfigFile, &needsWriteBack))
    {
        ErrorLog("FixShoppingWages ERROR: failed to read mod-config.json; using defaults and rewriting file");
        gConfigNeedsWriteBack = true;
        return;
    }

    gConfigNeedsWriteBack = (!foundConfigFile) || needsWriteBack;
    if (!foundConfigFile) { DebugLog("FixShoppingWages INFO: mod-config.json not found; using defaults"); }

    std::stringstream info;
    info << "FixShoppingWages INFO: loaded config enabled=" << (gConfig.enabled ? "true" : "false")
         << " settingsPath=\"" << gSettingsPath << "\""
         << " verboseDebugLogging=" << (gConfig.verboseDebugLogging ? "true" : "false")
         << " limitVerboseDebugLogging=" << (gConfig.limitVerboseDebugLogging ? "true" : "false")
         << " developerDebug=" << (gConfig.developerDebug ? "true" : "false")
         << " baseWageFallback=" << gConfig.baseWageFallback
         << " baseWageOverride=" << (gConfig.baseWageOverride ? "true" : "false")
         << " baseWageOverrideValue=" << gConfig.baseWageOverrideValue
         << " maxSavingsMultiplier=" << gConfig.maxSavingsMultiplier;
    DebugLog(info.str().c_str());
}

static bool SaveConfigState()
{
    if (gSettingsPath.empty())
    {
        ErrorLog("FixShoppingWages ERROR: settings path is empty; cannot save mod-config.json");
        return false;
    }

    if (!SaveConfigToFile(gSettingsPath, gConfig))
    {
        std::stringstream error;
        error << "FixShoppingWages ERROR: failed to save mod-config.json path=\"" << gSettingsPath << "\"";
        ErrorLog(error.str().c_str());
        return false;
    }

    DebugLog("FixShoppingWages INFO: saved mod-config.json");
    return true;
}

// -----------------------------------------------------------------------
// Emkej's Mod Core (Mod Hub) integration (inlined)
// -----------------------------------------------------------------------
#include "FixShoppingWagesModHub.inl"

// -----------------------------------------------------------------------
// Hooks and plugin entry point (inlined from FixShoppingWagesHooks.inl)
// -----------------------------------------------------------------------
#include "FixShoppingWagesHooks.inl"
