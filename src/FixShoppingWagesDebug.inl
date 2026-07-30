// -----------------------------------------------------------------------
// FixShoppingWagesDebug.inl. Diagnostic logging and snapshotting
// Included inline by FixShoppingWages.cpp
// -----------------------------------------------------------------------

// Set to false for production. Disables all diagnostic logging and snapshotting.
static const bool kVerboseDebugLogging = true;
// Set to true to listen for debug hotkeys and print their respective response.
static const bool kDeveloperDebug = true;

// -----------------------------------------------------------------------
// Hotkey edge-detection state
// -----------------------------------------------------------------------
static bool g_ctrlTPressedLast = false;      // CTRL+T
static bool g_ctrlShiftTPressedLast = false; // CTRL+SHIFT+T
static bool g_ctrlRPressedLast = false;      // CTRL+R

// -----------------------------------------------------------------------
// Day transition tracking
// -----------------------------------------------------------------------
static int g_previousDay = -1;

// -----------------------------------------------------------------------
// Hourly money snapshot tracking
// -----------------------------------------------------------------------
struct HourlyMoneyEntry
{
    std::string name;
    int money;
};

static std::map<hand, HourlyMoneyEntry> g_previousMoneySnapshot;
static int g_previousHour = -1;

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
    if (currentDay != g_previousDay)
    {
        double totalHours = ou->getTimeStamp_inGameHours().getTotalHours();
        std::stringstream message;
        message << "[updateUT] day transition: "
                << "day " << g_previousDay << " -> " << currentDay << " "
                << "(totalHours=" << totalHours << ")";
        DebugLog(message.str().c_str());
        g_previousDay = currentDay;
    }
}

// -----------------------------------------------------------------------
// Debug: hourly money snapshot diff
// -----------------------------------------------------------------------
static void LogHourlySnapshotDiff()
{
    double totalHours = ou->getTimeStamp_inGameHours().getTotalHours();
    int currentHour = static_cast<int>(totalHours);

    if (currentHour == g_previousHour) { return; }

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
            currentSnapshot[characterHandle] = entry;
        }
    }

    if (g_previousHour != -1)
    {
        int diffCount = 0;

        // Diff: check previous entries against current
        for (std::map<hand, HourlyMoneyEntry>::const_iterator previousIterator = g_previousMoneySnapshot.begin();
             previousIterator != g_previousMoneySnapshot.end(); ++previousIterator)
        {
            std::map<hand, HourlyMoneyEntry>::const_iterator currentIterator =
                currentSnapshot.find(previousIterator->first);

            if (currentIterator == currentSnapshot.end())
            {
                std::stringstream message;
                message << "[HOURLY DIFF] REMOVED: name=\"" << previousIterator->second.name << "\""
                        << " (id=\"" << previousIterator->first.toString() << "\")"
                        << " had=" << previousIterator->second.money << " hour=" << currentHour;
                DebugLog(message.str().c_str());
            }
            else if (currentIterator->second.money != previousIterator->second.money)
            {
                int delta = currentIterator->second.money - previousIterator->second.money;
                std::stringstream message;
                message << "[HOURLY DIFF] name=\"" << previousIterator->second.name << "\""
                        << " (id=\"" << previousIterator->first.toString() << "\")"
                        << " " << previousIterator->second.money << " -> " << currentIterator->second.money
                        << " (delta=" << delta << ")"
                        << " hour=" << currentHour;
                DebugLog(message.str().c_str());
                ++diffCount;
            }
        }

        // Check for new entries not in previous snapshot
        for (std::map<hand, HourlyMoneyEntry>::const_iterator currentIterator = currentSnapshot.begin();
             currentIterator != currentSnapshot.end(); ++currentIterator)
        {
            if (g_previousMoneySnapshot.find(currentIterator->first) == g_previousMoneySnapshot.end())
            {
                std::stringstream message;
                message << "[HOURLY DIFF] NEW: name=\"" << currentIterator->second.name << "\""
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

    g_previousMoneySnapshot = currentSnapshot;
    g_previousHour = currentHour;
}

// -----------------------------------------------------------------------
// Debug: hotkeys (CTRL+T, CTRL+SHIFT+T, CTRL+R)
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

    // --- CTRL+T: print NPC money + wages + money min/max ----------------
    if (controlT && !g_ctrlTPressedLast)
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
    g_ctrlTPressedLast = controlT;

    // --- CTRL+SHIFT+T: add 2000 money, then log ------------------------
    if (controlShiftT && !g_ctrlShiftTPressedLast)
    {
        Character *selectedCharacter = ou->player->selectedObject.getCharacter();
        if (selectedCharacter != nullptr)
        {
            int moneyBefore = selectedCharacter->getMoney();
            selectedCharacter->takeMoney(-2000); // negative = give money
            int moneyAfter = selectedCharacter->getMoney();

            std::stringstream message;
            message << "CTRL+SHIFT+T: "
                    << "name=\"" << selectedCharacter->getName() << "\" "
                    << "id=\"" << selectedCharacter->getHandle().toString() << "\" "
                    << "before=" << moneyBefore << " "
                    << "gave=2000 "
                    << "after=" << moneyAfter;
            DebugLog(message.str().c_str());
        }
        else
        {
            DebugLog("CTRL+SHIFT+T: no character selected");
        }
    }
    g_ctrlShiftTPressedLast = controlShiftT;

    // --- CTRL+R: dump all GameData fields ------------------------------
    if (controlR && !g_ctrlRPressedLast)
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
                    for (FloatDataMap::const_iterator iterator = floatMap.begin(); iterator != floatMap.end();
                         ++iterator)
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
                        lineMessage << "  [bdata] " << iterator->first << " = "
                                    << (iterator->second ? "true" : "false");
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
    g_ctrlRPressedLast = controlR;
}