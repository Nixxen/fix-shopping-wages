// -----------------------------------------------------------------------
// FixShoppingWagesHooks.inl. Hook implementations and plugin entry point
// Included inline by FixShoppingWages.cpp
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// DllMain: resolve config file path (runs before startPlugin)
// -----------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        char dllPath[_MAX_PATH] = {0};
        if (GetModuleFileNameA(hModule, dllPath, _MAX_PATH) > 0)
        {
            std::string fullPath = TrimAscii(std::string(dllPath));
            size_t sep = fullPath.find_last_of("\\/");
            if (sep != std::string::npos) { gSettingsPath = fullPath.substr(0, sep) + "\\" + kConfigFileName; }
        }
    }
    return TRUE;
}

// -----------------------------------------------------------------------
// Hook: GameWorld::dailyUpdates. Give NPCs their daily wage allowance
// -----------------------------------------------------------------------
static void (*GameWorld_dailyUpdates_originalFunction)(GameWorld *) = nullptr;

void GameWorld_dailyUpdates_hook(GameWorld *thisWorld)
{
    if (gConfig.verboseDebugLogging) { LogDailyUpdatesStart(thisWorld); }

    GameWorld_dailyUpdates_originalFunction(thisWorld);
    // NOTE for others reading this:
    //  The original GameWorld::dailyUpdates() function does not call the Character::dailyUpdate() function.
    //  We intentionally do not call the character dailyUpdate() function as well, since it overwrites the characters
    //  money by their set wage instead of adding to it. In other words, anyone with a set wage will have their money
    //  overwritten by the dailyUpdate() function, which is not what we want. It also ruins the vendors in the game,
    //  rendering them close to broke.

    // Resolve universal wage: if override is active, use the override value directly;
    // otherwise get the wage from one Dried Meat (may fluctuate per day).
    int resolvedUniversalWage = -1;
    if (gConfig.baseWageOverride) { resolvedUniversalWage = gConfig.baseWageOverrideValue; }
    else
    {
        resolvedUniversalWage = ResolveUniversalWage();
    }

    typedef ogre_unordered_set<Character *>::type CharacterSet;
    const CharacterSet &characterList = ou->getCharacterUpdateList();

    int nonPlayerCharacterCount = 0;
    int changedCharacterCount = 0;
    int fallbackWage = (resolvedUniversalWage != -1) ? resolvedUniversalWage : gConfig.baseWageFallback;

    for (CharacterSet::const_iterator iterator = characterList.begin(); iterator != characterList.end(); ++iterator)
    {
        Character *character = *iterator;
        if (character->isPlayerCharacter()) { continue; }

        DailyUpdateSnapshot beforeSnapshot;
        if (gConfig.verboseDebugLogging) { CaptureSnapshot(character, beforeSnapshot); }

        GiveDailyWage(character, fallbackWage, changedCharacterCount, beforeSnapshot);
        ++nonPlayerCharacterCount;
    }

    if (gConfig.verboseDebugLogging) { LogDailyUpdatesEnd(nonPlayerCharacterCount, changedCharacterCount, thisWorld); }
}

// -----------------------------------------------------------------------
// Hook: PlayerInterface::updateUT
// -----------------------------------------------------------------------
static void (*PlayerInterface_updateUT_originalFunction)(PlayerInterface *) = nullptr;

void PlayerInterface_updateUT_hook(PlayerInterface *thisPointer)
{
    PlayerInterface_updateUT_originalFunction(thisPointer);

    if (gConfig.verboseDebugLogging)
    {
        LogDayTransition();
        LogHourlySnapshotDiff();
    }
    if (gConfig.developerDebug) { LogHotkeys(); }
}

// -----------------------------------------------------------------------
// Plugin entry point
// -----------------------------------------------------------------------
__declspec(dllexport) void startPlugin()
{
    DebugLog("startPlugin()");

    LoadConfigState();
    if (gConfigNeedsWriteBack)
    {
        if (!SaveConfigState()) { ErrorLog("FixShoppingWages WARN: failed to persist normalized mod-config.json"); }
    }

    if (!gConfig.enabled)
    {
        DebugLog("Disabled by config; no hooks installed");
        return;
    }

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

    std::stringstream info;
    info << "INFO: initialized enabled=" << (gConfig.enabled ? "true" : "false")
         << " baseWageFallback=" << gConfig.baseWageFallback << " maxSavingsMultiplier=" << gConfig.maxSavingsMultiplier
         << " verboseDebugLogging=" << (gConfig.verboseDebugLogging ? "true" : "false")
         << " limitVerboseDebugLogging=" << (gConfig.limitVerboseDebugLogging ? "true" : "false")
         << " developerDebug=" << (gConfig.developerDebug ? "true" : "false");
    DebugLog(info.str().c_str());
}