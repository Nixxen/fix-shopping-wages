// -----------------------------------------------------------------------
// FixShoppingWagesHooks.inl. Hook implementations and plugin entry point
// Included inline by FixShoppingWages.cpp
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Hook: GameWorld::dailyUpdates. Give NPCs their daily wage allowance
// -----------------------------------------------------------------------
static void (*GameWorld_dailyUpdates_originalFunction)(GameWorld *) = nullptr;

void GameWorld_dailyUpdates_hook(GameWorld *thisWorld)
{
    if (kVerboseDebugLogging) { LogDailyUpdatesStart(thisWorld); }

    GameWorld_dailyUpdates_originalFunction(thisWorld);
    // NOTE for others reading this:
    //  The original GameWorld::dailyUpdates() function does not call the Character::dailyUpdate() function.
    //  We intentionally do not call the character dailyUpdate() function as well, since it overwrites the characters
    //  money by their set wage instead of adding to it. In other words, anyone with a set wage will have their money
    //  overwritten by the dailyUpdate() function, which is not what we want. It also ruins the vendors in the game,
    //  rendering them close to broke.

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

        GiveDailyWage(character, fallbackWage, changedCharacterCount, beforeSnapshot);
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