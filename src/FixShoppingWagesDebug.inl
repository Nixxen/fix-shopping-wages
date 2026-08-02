// -----------------------------------------------------------------------
// FixShoppingWagesDebug.inl. Diagnostic logging and snapshotting
// Included inline by FixShoppingWages.cpp
// -----------------------------------------------------------------------

// Enables excessively diagnostic logging and snapshotting.
#define kVerboseDebugLogging gConfig.verboseDebugLogging
// Tune down the excessive logging and only log the most important events.
#define kLimitVerboseDebugLogging gConfig.limitVerboseDebugLogging
// Enable direct character debugging. Allows injecting money into a character.
#define kDeveloperDebug gConfig.developerDebug

// -----------------------------------------------------------------------
// Hotkey edge-detection state
// -----------------------------------------------------------------------
static bool gCtrlTPressedLast = false;      // CTRL+T
static bool gCtrlShiftTPressedLast = false; // CTRL+SHIFT+T
static bool gCtrlRPressedLast = false;      // CTRL+R
static bool gCtrlShiftRPressedLast = false; // CTRL+SHIFT+R

// -----------------------------------------------------------------------
// Day transition tracking
// -----------------------------------------------------------------------
static int gPreviousDay = -1;

// -----------------------------------------------------------------------
// Hourly money snapshot tracking
// -----------------------------------------------------------------------
struct HourlyMoneyEntry
{
    std::string name;
    int money;
    int wages;
    int moneyMin;
    int moneyMax;
};

static std::map<hand, HourlyMoneyEntry> gPreviousMoneySnapshot;
static int gPreviousHour = -1;

// -----------------------------------------------------------------------
// Snapshot of Character fields to compare before/after wage update
// -----------------------------------------------------------------------
struct DailyUpdateSnapshot
{
    int money;
    bool stealthMode;
    int proneState;
    bool isCurrentlyGettingUp;
    int isGettingEaten;
    bool isEngagedWithAPlayer;
    int inSomething;
    bool isChained;
    bool isCarryingSomething;
    float frameTime;
    float lightLevel;
    float terrainHeightPosition;
    int armourType;
    float diplomacyMultiplier;
};

static void CaptureSnapshot(Character *character, DailyUpdateSnapshot &snapshot)
{
    snapshot.money = character->getMoney();
    snapshot.stealthMode = character->stealthMode;
    snapshot.proneState = static_cast<int>(character->_currentProneState);
    snapshot.isCurrentlyGettingUp = character->isCurrentlyGettingUp;
    snapshot.isGettingEaten = static_cast<int>(character->isGettingEaten);
    snapshot.isEngagedWithAPlayer = character->_isEngagedWithAPlayer;
    snapshot.inSomething = static_cast<int>(character->inSomething);
    snapshot.isChained = character->isChained;
    snapshot.isCarryingSomething = character->isCarryingSomething;
    snapshot.frameTime = character->frameTIME;
    snapshot.lightLevel = character->_lightLevel;
    snapshot.terrainHeightPosition = character->terrainHeightPosition;
    snapshot.armourType = static_cast<int>(character->armourType);
    snapshot.diplomacyMultiplier = character->diplomacyMultiplier;
}

static bool BuildDiffString(const DailyUpdateSnapshot &before, const DailyUpdateSnapshot &after, std::string &diffOut)
{
    std::stringstream differences;
    bool anyChanged = false;

    if (before.money != after.money)
    {
        int delta = after.money - before.money;
        differences << " money:" << before.money << "->" << after.money << " (delta=" << delta << ")";
        anyChanged = true;
    }
    if (before.stealthMode != after.stealthMode)
    {
        differences << " stealthMode:" << before.stealthMode << "->" << after.stealthMode;
        anyChanged = true;
    }
    if (before.proneState != after.proneState)
    {
        differences << " proneState:" << before.proneState << "->" << after.proneState;
        anyChanged = true;
    }
    if (before.isCurrentlyGettingUp != after.isCurrentlyGettingUp)
    {
        differences << " isCurrentlyGettingUp:" << before.isCurrentlyGettingUp << "->" << after.isCurrentlyGettingUp;
        anyChanged = true;
    }
    if (before.isGettingEaten != after.isGettingEaten)
    {
        differences << " isGettingEaten:" << before.isGettingEaten << "->" << after.isGettingEaten;
        anyChanged = true;
    }
    if (before.isEngagedWithAPlayer != after.isEngagedWithAPlayer)
    {
        differences << " isEngagedWithAPlayer:" << before.isEngagedWithAPlayer << "->" << after.isEngagedWithAPlayer;
        anyChanged = true;
    }
    if (before.inSomething != after.inSomething)
    {
        differences << " inSomething:" << before.inSomething << "->" << after.inSomething;
        anyChanged = true;
    }
    if (before.isChained != after.isChained)
    {
        differences << " isChained:" << before.isChained << "->" << after.isChained;
        anyChanged = true;
    }
    if (before.isCarryingSomething != after.isCarryingSomething)
    {
        differences << " isCarryingSomething:" << before.isCarryingSomething << "->" << after.isCarryingSomething;
        anyChanged = true;
    }
    if (before.frameTime != after.frameTime)
    {
        int delta = static_cast<int>(after.frameTime - before.frameTime);
        differences << " frameTime:" << before.frameTime << "->" << after.frameTime << " (delta=" << delta << ")";
        anyChanged = true;
    }
    if (before.lightLevel != after.lightLevel)
    {
        int delta = static_cast<int>(after.lightLevel - before.lightLevel);
        differences << " lightLevel:" << before.lightLevel << "->" << after.lightLevel << " (delta=" << delta << ")";
        anyChanged = true;
    }
    if (before.terrainHeightPosition != after.terrainHeightPosition)
    {
        int delta = static_cast<int>(after.terrainHeightPosition - before.terrainHeightPosition);
        differences << " terrainHeight:" << before.terrainHeightPosition << "->" << after.terrainHeightPosition
                    << " (delta=" << delta << ")";
        anyChanged = true;
    }
    if (before.armourType != after.armourType)
    {
        differences << " armourType:" << before.armourType << "->" << after.armourType;
        anyChanged = true;
    }
    if (before.diplomacyMultiplier != after.diplomacyMultiplier)
    {
        differences << " diplomacyMult:" << before.diplomacyMultiplier << "->" << after.diplomacyMultiplier;
        anyChanged = true;
    }

    if (anyChanged) { diffOut = differences.str(); }
    return anyChanged;
}

// -----------------------------------------------------------------------
// Debug: dailyUpdate START message
// -----------------------------------------------------------------------
static void LogDailyUpdatesStart(GameWorld *thisWorld)
{
    TimeOfDay currentTime = thisWorld->getTimeStamp_inGameHours();
    double totalHours = currentTime.getTotalHours();
    double totalDays = currentTime.getTotalDays();

    std::stringstream message;
    message << "[GameWorld] dailyUpdates START: "
            << "totalHours=" << totalHours << " "
            << "totalDays=" << totalDays;
    DebugLog(message.str().c_str());
}

// -----------------------------------------------------------------------
// Debug: per-character dailyUpdate diff
// -----------------------------------------------------------------------
static void
LogDailyUpdateCharacterDiff(Character *character, const DailyUpdateSnapshot &beforeSnapshot, int &changedCharacterCount)
{
    DailyUpdateSnapshot afterSnapshot;
    CaptureSnapshot(character, afterSnapshot);

    GameData *characterData = character->data;
    int wagesValue = LookupGameDataInteger(characterData, "wages", -1);
    int moneyMin = LookupGameDataInteger(characterData, "money min", -1);
    int moneyMax = LookupGameDataInteger(characterData, "money max", -1);

    std::string diffString;
    if (BuildDiffString(beforeSnapshot, afterSnapshot, diffString))
    {
        std::stringstream message;
        message << "[DAILYUPDATE DIFF] "
                << "name=\"" << character->getName() << "\""
                << " id=\"" << character->getHandle().toString() << "\"" << diffString << " wages=" << wagesValue
                << " moneyMin=" << moneyMin << " moneyMax=" << moneyMax;
        DebugLog(message.str().c_str());
        ++changedCharacterCount;
    }
    else
    {
        std::stringstream message;
        message << "[DAILYUPDATE NODIFF] "
                << "name=\"" << character->getName() << "\""
                << " id=\"" << character->getHandle().toString() << "\""
                << " money=" << afterSnapshot.money << " wages=" << wagesValue << " moneyMin=" << moneyMin
                << " moneyMax=" << moneyMax;
        DebugLog(message.str().c_str());
    }
}

// -----------------------------------------------------------------------
// Debug: dailyUpdate END message
// -----------------------------------------------------------------------
static void LogDailyUpdatesEnd(int nonPlayerCharacterCount, int changedCharacterCount, GameWorld *thisWorld)
{
    TimeOfDay currentTime = thisWorld->getTimeStamp_inGameHours();
    double totalHours = currentTime.getTotalHours();
    double totalDays = currentTime.getTotalDays();

    std::stringstream message;
    message << "[GameWorld] dailyUpdates END: "
            << "npcCount=" << nonPlayerCharacterCount << " "
            << "changedCount=" << changedCharacterCount << " "
            << "totalHours=" << totalHours << " "
            << "totalDays=" << totalDays;
    DebugLog(message.str().c_str());
}

// -----------------------------------------------------------------------
// Debug: day transition detector
// -----------------------------------------------------------------------
static void LogDayTransition()
{
    int currentDay = static_cast<int>(ou->getTimeStamp_inGameHours().getTotalDays());
    if (currentDay != gPreviousDay)
    {
        double totalHours = ou->getTimeStamp_inGameHours().getTotalHours();
        std::stringstream message;
        message << "[updateUT] day transition: "
                << "day " << gPreviousDay << " -> " << currentDay << " "
                << "(totalHours=" << totalHours << ")";
        DebugLog(message.str().c_str());
        gPreviousDay = currentDay;
    }
}

// -----------------------------------------------------------------------
// Debug: hourly money snapshot diff
// -----------------------------------------------------------------------
static void LogHourlySnapshotDiff()
{
    double totalHours = ou->getTimeStamp_inGameHours().getTotalHours();
    int currentHour = static_cast<int>(totalHours);
    static const int kHoursPerDay = 24;
    int dayHour = currentHour % kHoursPerDay;

    if (currentHour == gPreviousHour) { return; }

    typedef ogre_unordered_set<Character *>::type CharacterSet;
    const CharacterSet &characterList = ou->getCharacterUpdateList();

    // Build current hour's snapshot
    std::map<hand, HourlyMoneyEntry> currentSnapshot;
    for (CharacterSet::const_iterator iterator = characterList.begin(); iterator != characterList.end(); ++iterator)
    {
        Character *character = *iterator;
        if (!character->isPlayerCharacter())
        {
            hand characterHandle(character);
            HourlyMoneyEntry entry;
            entry.name = character->getName();
            entry.money = character->getMoney();
            entry.wages = LookupGameDataInteger(character->data, "wages", -1);
            entry.moneyMin = LookupGameDataInteger(character->data, "money min", -1);
            entry.moneyMax = LookupGameDataInteger(character->data, "money max", -1);
            currentSnapshot[characterHandle] = entry;
        }
    }

    if (gPreviousHour != -1)
    {
        int diffCount = 0;

        // Diff: check previous entries against current
        for (std::map<hand, HourlyMoneyEntry>::const_iterator previousIterator = gPreviousMoneySnapshot.begin();
             previousIterator != gPreviousMoneySnapshot.end(); ++previousIterator)
        {
            std::map<hand, HourlyMoneyEntry>::const_iterator currentIterator =
                currentSnapshot.find(previousIterator->first);

            if (currentIterator == currentSnapshot.end())
            {
                if (kLimitVerboseDebugLogging)
                {
                    // Skip logging for removed characters if limiting verbose logging
                    continue;
                }
                std::stringstream message;
                message << "[HOURLY DIFF] REMOVED: dayhour=" << dayHour << " name=\"" << previousIterator->second.name
                        << "\""
                        << " (id=\"" << previousIterator->first.toString() << "\")"
                        << " had=" << previousIterator->second.money << " hour=" << currentHour;
                DebugLog(message.str().c_str());
            }
            else if (currentIterator->second.money != previousIterator->second.money)
            {
                int delta = currentIterator->second.money - previousIterator->second.money;
                std::stringstream message;
                message << "[HOURLY DIFF] dayhour=" << dayHour << " name=\"" << previousIterator->second.name << "\""
                        << " (id=\"" << previousIterator->first.toString() << "\")"
                        << " " << previousIterator->second.money << " -> " << currentIterator->second.money
                        << " (delta=" << delta << ", wages=" << currentIterator->second.wages
                        << ", moneyMin=" << currentIterator->second.moneyMin
                        << ", moneyMax=" << currentIterator->second.moneyMax << ")"
                        << " hour=" << currentHour;
                DebugLog(message.str().c_str());
                ++diffCount;
            }
        }

        // Check for new entries not in previous snapshot
        for (std::map<hand, HourlyMoneyEntry>::const_iterator currentIterator = currentSnapshot.begin();
             currentIterator != currentSnapshot.end(); ++currentIterator)
        {
            if (gPreviousMoneySnapshot.find(currentIterator->first) == gPreviousMoneySnapshot.end())
            {
                if (kLimitVerboseDebugLogging)
                {
                    // Skip logging for new characters if limiting verbose logging
                    continue;
                }
                std::stringstream message;
                message << "[HOURLY DIFF] NEW: dayhour=" << dayHour << " name=\"" << currentIterator->second.name
                        << "\""
                        << " (id=\"" << currentIterator->first.toString() << "\")"
                        << " money=" << currentIterator->second.money << " hour=" << currentHour;
                DebugLog(message.str().c_str());
            }
        }

        if (diffCount > 0)
        {
            std::stringstream message;
            message << "[HOURLY DIFF] " << diffCount << " NPC(s) had money changes this hour (hour=" << currentHour
                    << ")";
            DebugLog(message.str().c_str());
        }
    }

    gPreviousMoneySnapshot = currentSnapshot;
    gPreviousHour = currentHour;
}

// -----------------------------------------------------------------------
// Debug: hotkey action functions
// -----------------------------------------------------------------------

static void HotkeyLogCharacterMoney()
{
    Character *selectedCharacter = ou->player->selectedObject.getCharacter();
    if (selectedCharacter != nullptr)
    {
        int currentMoney = selectedCharacter->getMoney();
        GameData *characterData = selectedCharacter->data;
        int wagesValue = LookupGameDataInteger(characterData, "wages", -1);
        int moneyMin = LookupGameDataInteger(characterData, "money min", -1);
        int moneyMax = LookupGameDataInteger(characterData, "money max", -1);

        std::stringstream message;
        message << "CTRL+T: "
                << "name=\"" << selectedCharacter->getName() << "\" "
                << "id=\"" << selectedCharacter->getHandle().toString() << "\" "
                << "money=" << currentMoney << " "
                << "wages=" << wagesValue << " "
                << "moneyMin=" << moneyMin << " "
                << "moneyMax=" << moneyMax;
        DebugLog(message.str().c_str());
    }
    else
    {
        DebugLog("CTRL+T: no character selected");
    }
}

static void HotkeyGiveMoney()
{
    Character *selectedCharacter = ou->player->selectedObject.getCharacter();
    if (selectedCharacter != nullptr)
    {
        int moneyBefore = selectedCharacter->getMoney();
        static const int kMoneyToGive = 2000;
        selectedCharacter->takeMoney(-kMoneyToGive); // negative = give money
        int moneyAfter = selectedCharacter->getMoney();

        std::stringstream message;
        message << "CTRL+SHIFT+T: "
                << "name=\"" << selectedCharacter->getName() << "\" "
                << "id=\"" << selectedCharacter->getHandle().toString() << "\" "
                << "before=" << moneyBefore << " "
                << "gave=" << kMoneyToGive << " "
                << "after=" << moneyAfter;
        DebugLog(message.str().c_str());
    }
    else
    {
        DebugLog("CTRL+SHIFT+T: no character selected");
    }
}

static void HotkeyDumpCharacterGameData()
{
    Character *selectedCharacter = ou->player->selectedObject.getCharacter();
    if (selectedCharacter != nullptr)
    {
        GameData *characterData = selectedCharacter->data;
        if (characterData != nullptr)
        {
            std::stringstream headerMessage;

            headerMessage << "CTRL+R: name=\"" << selectedCharacter->getName() << "\""
                          << " type=" << static_cast<int>(characterData->type) << " stringID=\""
                          << characterData->stringID << "\"";

            DebugLog(headerMessage.str().c_str());

            // -- integer fields (idata) --
            {
                typedef boost::unordered::unordered_map<
                    std::string, int, boost::hash<std::string>, std::equal_to<std::string>,
                    Ogre::STLAllocator<std::pair<const std::string, int>, Ogre::GeneralAllocPolicy>>
                    IntegerDataMap;

                IntegerDataMap &integerMap = characterData->idata;
                for (IntegerDataMap::const_iterator iterator = integerMap.begin(); iterator != integerMap.end();
                     ++iterator)
                {
                    std::stringstream lineMessage;
                    lineMessage << "  [idata] " << iterator->first << " = " << iterator->second;
                    DebugLog(lineMessage.str().c_str());
                }
            }

            // -- float fields (fdata) --
            {
                typedef boost::unordered::unordered_map<
                    std::string, float, boost::hash<std::string>, std::equal_to<std::string>,
                    Ogre::STLAllocator<std::pair<const std::string, float>, Ogre::GeneralAllocPolicy>>
                    FloatDataMap;

                FloatDataMap &floatMap = characterData->fdata;
                for (FloatDataMap::const_iterator iterator = floatMap.begin(); iterator != floatMap.end(); ++iterator)
                {
                    std::stringstream lineMessage;
                    lineMessage << "  [fdata] " << iterator->first << " = " << iterator->second;
                    DebugLog(lineMessage.str().c_str());
                }
            }

            // -- string fields (sdata) --
            {
                typedef boost::unordered::unordered_map<
                    std::string, std::string, boost::hash<std::string>, std::equal_to<std::string>,
                    Ogre::STLAllocator<std::pair<const std::string, std::string>, Ogre::GeneralAllocPolicy>>
                    StringDataMap;

                StringDataMap &stringMap = characterData->sdata;
                for (StringDataMap::const_iterator iterator = stringMap.begin(); iterator != stringMap.end();
                     ++iterator)
                {
                    std::stringstream lineMessage;
                    lineMessage << "  [sdata] " << iterator->first << " = \"" << iterator->second << "\"";
                    DebugLog(lineMessage.str().c_str());
                }
            }

            // -- bool fields (bdata) --
            {
                typedef boost::unordered::unordered_map<
                    std::string, bool, boost::hash<std::string>, std::equal_to<std::string>,
                    Ogre::STLAllocator<std::pair<const std::string, bool>, Ogre::GeneralAllocPolicy>>
                    BoolDataMap;

                BoolDataMap &boolMap = characterData->bdata;
                for (BoolDataMap::const_iterator iterator = boolMap.begin(); iterator != boolMap.end(); ++iterator)
                {
                    std::stringstream lineMessage;
                    lineMessage << "  [bdata] " << iterator->first << " = " << (iterator->second ? "true" : "false");
                    DebugLog(lineMessage.str().c_str());
                }
            }

            DebugLog("CTRL+R: -- end of GameData dump --");
        }
        else
        {
            DebugLog("CTRL+R: selected character has no GameData");
        }
    }
    else
    {
        DebugLog("CTRL+R: no character selected");
    }
}

static void HotkeyLookupDriedMeatPrice()
{
    // Dump static game data price for Dried Meat.
    DebugLog("CTRL+SHIFT+R: looking up Dried Meat from ou->gamedata...");

    const std::string kDriedMeatName = "Dried Meat";
    GameData *itemData = ou->gamedata.getDataByName(kDriedMeatName, ITEM);

    if (itemData == nullptr)
    {
        DebugLog("CTRL+SHIFT+R: could not find Dried Meat by name, trying by string ID...");
        itemData = ou->gamedata.getData("46130-DriedMeat", ITEM);
    }
    if (itemData == nullptr)
    {
        DebugLog("CTRL+SHIFT+R: could not find Dried Meat in ou->gamedata by string ID, trying by numeric ID...");
    }
    else
    {
        std::stringstream headerMessage;
        headerMessage << "CTRL+SHIFT+R: found GameData stringID=\"" << itemData->stringID
                      << "\" type=" << static_cast<int>(itemData->type);
        DebugLog(headerMessage.str().c_str());

        // Read price from idata
        typedef boost::unordered::unordered_map<
            std::string, int, boost::hash<std::string>, std::equal_to<std::string>,
            Ogre::STLAllocator<std::pair<const std::string, int>, Ogre::GeneralAllocPolicy>>
            IntegerDataMap;

        IntegerDataMap &integerMap = itemData->idata;
        IntegerDataMap::const_iterator priceIter = integerMap.find("value");
        if (priceIter != integerMap.end())
        {
            std::stringstream lineMessage;
            lineMessage << "  [idata] value = " << priceIter->second << " (price)";
            DebugLog(lineMessage.str().c_str());
        }
        else
        {
            DebugLog("  [idata] key \"value\" not found. Dumping all idata keys:");
            for (IntegerDataMap::const_iterator iterator = integerMap.begin(); iterator != integerMap.end(); ++iterator)
            {
                std::stringstream lineMessage;
                lineMessage << "  [idata] " << iterator->first << " = " << iterator->second;
                DebugLog(lineMessage.str().c_str());
            }
        }
    }
    // Dump live item price for Dried Meat by spawning a temporary instance and reading its price.
    DebugLog("CTRL+SHIFT+R: spawning temporary Dried Meat instance from GameData to read live price...");
    if (itemData != nullptr)
    {
        Item *item = ou->theFactory->createItem(
            itemData, // the GameData we looked up
            hand(),   // null handle
            nullptr,  // no weapon mesh override
            nullptr,  // default material
            -1,       // Assumption: default level
            nullptr   // no faction uniform
        );
        if (item != nullptr)
        {
            int livePrice = item->getAvgPrice();
            std::stringstream liveMsg;
            liveMsg << "CTRL+SHIFT+R: spawned instance getAvgPrice() = " << livePrice;
            DebugLog(liveMsg.str().c_str());
            ou->destroy(item, false, "debug query");
        }
        else
        {
            DebugLog("CTRL+SHIFT+R: createItem returned NULL");
        }
    }
    else
    {
        DebugLog("CTRL+SHIFT+R: no itemData to spawn from");
    }
}

// -----------------------------------------------------------------------
// Debug: hotkeys dispatch
// -----------------------------------------------------------------------
static void LogHotkeys()
{
    const bool controlKeyDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shiftKeyDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool tKeyDown = (GetAsyncKeyState('T') & 0x8000) != 0;
    const bool rKeyDown = (GetAsyncKeyState('R') & 0x8000) != 0;

    const bool controlT = controlKeyDown && !shiftKeyDown && tKeyDown;
    const bool controlShiftT = controlKeyDown && shiftKeyDown && tKeyDown;
    const bool controlR = controlKeyDown && !shiftKeyDown && rKeyDown;
    const bool controlShiftR = controlKeyDown && shiftKeyDown && rKeyDown;

    if (controlT && !gCtrlTPressedLast) { HotkeyLogCharacterMoney(); }
    gCtrlTPressedLast = controlT;

    if (controlShiftT && !gCtrlShiftTPressedLast) { HotkeyGiveMoney(); }
    gCtrlShiftTPressedLast = controlShiftT;

    if (controlR && !gCtrlRPressedLast) { HotkeyDumpCharacterGameData(); }
    gCtrlRPressedLast = controlR;

    if (controlShiftR && !gCtrlShiftRPressedLast) { HotkeyLookupDriedMeatPrice(); }
    gCtrlShiftRPressedLast = controlShiftR;
}
