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

#include <sstream>
#include <string>

// -----------------------------------------------------------------------
// Helper: look up an integer value from GameData::idata by key
// -----------------------------------------------------------------------
static int LookupGameDataInteger(GameData *gameData, const std::string &key,
                                 int defaultValue) {
  if (gameData == nullptr) {
    return defaultValue;
  }

  typedef boost::unordered::unordered_map<
      std::string, int, boost::hash<std::string>, std::equal_to<std::string>,
      Ogre::STLAllocator<std::pair<const std::string, int>,
                         Ogre::GeneralAllocPolicy>>
      IntegerDataMap;

  IntegerDataMap &integerMap = gameData->idata;
  IntegerDataMap::const_iterator iterator = integerMap.find(key);
  if (iterator != integerMap.end()) {
    return iterator->second;
  }
  return defaultValue;
}

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
// Snapshot of Character fields to compare before/after dailyUpdate
// -----------------------------------------------------------------------
struct DailyUpdateSnapshot {
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
  float diplomaMultiplier;
};

static void CaptureSnapshot(Character *character,
                            DailyUpdateSnapshot &snapshot) {
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
  snapshot.diplomaMultiplier = character->diplomacyMultiplier;
}

static bool BuildDiffString(const DailyUpdateSnapshot &before,
                            const DailyUpdateSnapshot &after,
                            std::string &diffOut) {
  std::stringstream differences;
  bool anyChanged = false;

  if (before.money != after.money) {
    differences << " money:" << before.money << "->" << after.money;
    anyChanged = true;
  }
  if (before.stealthMode != after.stealthMode) {
    differences << " stealthMode:" << before.stealthMode << "->"
                << after.stealthMode;
    anyChanged = true;
  }
  if (before.proneState != after.proneState) {
    differences << " proneState:" << before.proneState << "->"
                << after.proneState;
    anyChanged = true;
  }
  if (before.isCurrentlyGettingUp != after.isCurrentlyGettingUp) {
    differences << " isCurrentlyGettingUp:" << before.isCurrentlyGettingUp
                << "->" << after.isCurrentlyGettingUp;
    anyChanged = true;
  }
  if (before.isGettingEaten != after.isGettingEaten) {
    differences << " isGettingEaten:" << before.isGettingEaten << "->"
                << after.isGettingEaten;
    anyChanged = true;
  }
  if (before.isEngagedWithAPlayer != after.isEngagedWithAPlayer) {
    differences << " isEngagedWithAPlayer:" << before.isEngagedWithAPlayer
                << "->" << after.isEngagedWithAPlayer;
    anyChanged = true;
  }
  if (before.inSomething != after.inSomething) {
    differences << " inSomething:" << before.inSomething << "->"
                << after.inSomething;
    anyChanged = true;
  }
  if (before.isChained != after.isChained) {
    differences << " isChained:" << before.isChained << "->" << after.isChained;
    anyChanged = true;
  }
  if (before.isCarryingSomething != after.isCarryingSomething) {
    differences << " isCarryingSomething:" << before.isCarryingSomething << "->"
                << after.isCarryingSomething;
    anyChanged = true;
  }
  if (before.frameTime != after.frameTime) {
    differences << " frameTime:" << before.frameTime << "->" << after.frameTime;
    anyChanged = true;
  }
  if (before.lightLevel != after.lightLevel) {
    differences << " lightLevel:" << before.lightLevel << "->"
                << after.lightLevel;
    anyChanged = true;
  }
  if (before.terrainHeightPosition != after.terrainHeightPosition) {
    differences << " terrainHeight:" << before.terrainHeightPosition << "->"
                << after.terrainHeightPosition;
    anyChanged = true;
  }
  if (before.armourType != after.armourType) {
    differences << " armourType:" << before.armourType << "->"
                << after.armourType;
    anyChanged = true;
  }
  if (before.diplomaMultiplier != after.diplomaMultiplier) {
    differences << " diplomacyMult:" << before.diplomaMultiplier << "->"
                << after.diplomaMultiplier;
    anyChanged = true;
  }

  if (anyChanged) {
    diffOut = differences.str();
  }
  return anyChanged;
}

// -----------------------------------------------------------------------
// Hook: Character::dailyUpdate (log ALL calls, no filter)
// -----------------------------------------------------------------------
static void (*Character_dailyUpdate_originalFunction)(Character *) = nullptr;

void Character_dailyUpdate_hook(Character *thisCharacter) {
  Character_dailyUpdate_originalFunction(thisCharacter);

  int currentMoney = thisCharacter->getMoney();
  std::stringstream message;
  message << "[Character] dailyUpdate: "
          << "name=\"" << thisCharacter->getName() << "\" "
          << "money=" << currentMoney;
  DebugLog(message.str().c_str());
}

// -----------------------------------------------------------------------
// Hook: GameWorld::dailyUpdates (triggers dailyUpdate on all NPCs)
// -----------------------------------------------------------------------
static void (*GameWorld_dailyUpdates_originalFunction)(GameWorld *) = nullptr;

void GameWorld_dailyUpdates_hook(GameWorld *thisWorld) {
  TimeOfDay currentTime = thisWorld->getTimeStamp_inGameHours();
  double totalHours = currentTime.getTotalHours();
  double totalDays = currentTime.getTotalDays();

  std::stringstream beforeMessage;
  beforeMessage << "[GameWorld] dailyUpdates START: "
                << "totalHours=" << totalHours << " "
                << "totalDays=" << totalDays;
  DebugLog(beforeMessage.str().c_str());

  GameWorld_dailyUpdates_originalFunction(thisWorld);

  typedef ogre_unordered_set<Character *>::type CharacterSet;
  const CharacterSet &characterList = ou->getCharacterUpdateList();

  int nonPlayerCharacterCount = 0;
  int changedCharacterCount = 0;

  for (CharacterSet::const_iterator iterator = characterList.begin();
       iterator != characterList.end(); ++iterator) {
    Character *character = *iterator;
    if (character->isPlayerCharacter()) {
      continue;
    }

    DailyUpdateSnapshot beforeSnapshot;
    CaptureSnapshot(character, beforeSnapshot);

    character->dailyUpdate();

    DailyUpdateSnapshot afterSnapshot;
    CaptureSnapshot(character, afterSnapshot);

    GameData *characterData = character->data;
    int wagesValue = LookupGameDataInteger(characterData, "wages", -1);
    int moneyMin = LookupGameDataInteger(characterData, "money min", -1);
    int moneyMax = LookupGameDataInteger(characterData, "money max", -1);

    std::string diffString;
    if (BuildDiffString(beforeSnapshot, afterSnapshot, diffString)) {
      std::stringstream diffMessage;
      diffMessage << "[DAILYUPDATE DIFF] "
                  << "name=\"" << character->getName() << "\"" << diffString
                  << " wages=" << wagesValue << " moneyMin=" << moneyMin
                  << " moneyMax=" << moneyMax;
      DebugLog(diffMessage.str().c_str());
      ++changedCharacterCount;
    } else {
      std::stringstream noDiffMessage;
      noDiffMessage << "[DAILYUPDATE NODIFF] "
                    << "name=\"" << character->getName() << "\""
                    << " money=" << afterSnapshot.money
                    << " wages=" << wagesValue << " moneyMin=" << moneyMin
                    << " moneyMax=" << moneyMax;
      DebugLog(noDiffMessage.str().c_str());
    }

    ++nonPlayerCharacterCount;
  }

  std::stringstream afterMessage;
  afterMessage << "[GameWorld] dailyUpdates END: "
               << "npcCount=" << nonPlayerCharacterCount << " "
               << "changedCount=" << changedCharacterCount << " "
               << "totalHours=" << totalHours << " "
               << "totalDays=" << totalDays;
  DebugLog(afterMessage.str().c_str());
}

// -----------------------------------------------------------------------
// Hook: PlayerInterface::updateUT
// -----------------------------------------------------------------------
static void (*PlayerInterface_updateUT_originalFunction)(PlayerInterface *) =
    nullptr;

void PlayerInterface_updateUT_hook(PlayerInterface *thisPointer) {
  PlayerInterface_updateUT_originalFunction(thisPointer);

  // --- Day transition detector ---------------------------------------

  int currentDay =
      static_cast<int>(ou->getTimeStamp_inGameHours().getTotalDays());
  if (currentDay != g_previousDay) {
    double totalHours = ou->getTimeStamp_inGameHours().getTotalHours();
    std::stringstream dayMessage;
    dayMessage << "[updateUT] day transition: "
               << "day " << g_previousDay << " -> " << currentDay << " "
               << "(totalHours=" << totalHours << ")";
    DebugLog(dayMessage.str().c_str());
    g_previousDay = currentDay;
  }

  // --- Hotkeys -------------------------------------------------------

  const bool controlKeyDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
  const bool shiftKeyDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
  const bool tKeyDown = (GetAsyncKeyState('T') & 0x8000) != 0;
  const bool rKeyDown = (GetAsyncKeyState('R') & 0x8000) != 0;

  const bool controlT = controlKeyDown && !shiftKeyDown && tKeyDown;
  const bool controlShiftT = controlKeyDown && shiftKeyDown && tKeyDown;
  const bool controlR = controlKeyDown && !shiftKeyDown && rKeyDown;

  // --- CTRL+T: print NPC money + wages + money min/max ----------------
  if (controlT && !g_ctrlTPressedLast) {
    Character *selectedCharacter = ou->player->selectedObject.getCharacter();
    if (selectedCharacter != nullptr) {
      int currentMoney = selectedCharacter->getMoney();
      GameData *characterData = selectedCharacter->data;
      int wagesValue = LookupGameDataInteger(characterData, "wages", -1);
      int moneyMin = LookupGameDataInteger(characterData, "money min", -1);
      int moneyMax = LookupGameDataInteger(characterData, "money max", -1);

      std::stringstream message;
      message << "CTRL+T: "
              << "name=\"" << selectedCharacter->getName() << "\" "
              << "money=" << currentMoney << " "
              << "wages=" << wagesValue << " "
              << "moneyMin=" << moneyMin << " "
              << "moneyMax=" << moneyMax;
      DebugLog(message.str().c_str());
    } else {
      DebugLog("CTRL+T: no character selected");
    }
  }
  g_ctrlTPressedLast = controlT;

  // --- CTRL+SHIFT+T: add 2000 money, then log ------------------------
  if (controlShiftT && !g_ctrlShiftTPressedLast) {
    Character *selectedCharacter = ou->player->selectedObject.getCharacter();
    if (selectedCharacter != nullptr) {
      int moneyBefore = selectedCharacter->getMoney();
      selectedCharacter->takeMoney(-2000); // negative = give money
      int moneyAfter = selectedCharacter->getMoney();

      std::stringstream message;
      message << "CTRL+SHIFT+T: "
              << "name=\"" << selectedCharacter->getName() << "\" "
              << "before=" << moneyBefore << " "
              << "gave=2000 "
              << "after=" << moneyAfter;
      DebugLog(message.str().c_str());
    } else {
      DebugLog("CTRL+SHIFT+T: no character selected");
    }
  }
  g_ctrlShiftTPressedLast = controlShiftT;

  // --- CTRL+R: dump all GameData fields ------------------------------
  if (controlR && !g_ctrlRPressedLast) {
    Character *selectedCharacter = ou->player->selectedObject.getCharacter();
    if (selectedCharacter != nullptr) {
      GameData *characterData = selectedCharacter->data;
      if (characterData != nullptr) {
        std::stringstream headerMessage;

        headerMessage << "CTRL+R: name=\""
                      << selectedCharacter->getName() << "\""
                      << " type=" << static_cast<int>(characterData->type)
                      << " stringID=\"" << characterData->stringID << "\"";

        DebugLog(headerMessage.str().c_str());

        // -- integer fields (idata) --
        {
          typedef boost::unordered::unordered_map<
              std::string, int, boost::hash<std::string>,
              std::equal_to<std::string>,
              Ogre::STLAllocator<std::pair<const std::string, int>,
                                 Ogre::GeneralAllocPolicy>>
              IntegerDataMap;

          IntegerDataMap &integerMap = characterData->idata;
          for (IntegerDataMap::const_iterator iterator = integerMap.begin();
               iterator != integerMap.end(); ++iterator) {
            std::stringstream lineMessage;
            lineMessage << "  [idata] " << iterator->first << " = "
                        << iterator->second;
            DebugLog(lineMessage.str().c_str());
          }
        }

        // -- float fields (fdata) --
        {
          typedef boost::unordered::unordered_map<
              std::string, float, boost::hash<std::string>,
              std::equal_to<std::string>,
              Ogre::STLAllocator<std::pair<const std::string, float>,
                                 Ogre::GeneralAllocPolicy>>
              FloatDataMap;

          FloatDataMap &floatMap = characterData->fdata;
          for (FloatDataMap::const_iterator iterator = floatMap.begin();
               iterator != floatMap.end(); ++iterator) {
            std::stringstream lineMessage;
            lineMessage << "  [fdata] " << iterator->first << " = "
                        << iterator->second;
            DebugLog(lineMessage.str().c_str());
          }
        }

        // -- string fields (sdata) --
        {
          typedef boost::unordered::unordered_map<
              std::string, std::string, boost::hash<std::string>,
              std::equal_to<std::string>,
              Ogre::STLAllocator<std::pair<const std::string, std::string>,
                                 Ogre::GeneralAllocPolicy>>
              StringDataMap;

          StringDataMap &stringMap = characterData->sdata;
          for (StringDataMap::const_iterator iterator = stringMap.begin();
               iterator != stringMap.end(); ++iterator) {
            std::stringstream lineMessage;
            lineMessage << "  [sdata] " << iterator->first << " = \""
                        << iterator->second << "\"";
            DebugLog(lineMessage.str().c_str());
          }
        }

        // -- bool fields (bdata) --
        {
          typedef boost::unordered::unordered_map<
              std::string, bool, boost::hash<std::string>,
              std::equal_to<std::string>,
              Ogre::STLAllocator<std::pair<const std::string, bool>,
                                 Ogre::GeneralAllocPolicy>>
              BoolDataMap;

          BoolDataMap &boolMap = characterData->bdata;
          for (BoolDataMap::const_iterator iterator = boolMap.begin();
               iterator != boolMap.end(); ++iterator) {
            std::stringstream lineMessage;
            lineMessage << "  [bdata] " << iterator->first << " = "
                        << (iterator->second ? "true" : "false");
            DebugLog(lineMessage.str().c_str());
          }
        }

        DebugLog("CTRL+R: -- end of GameData dump --");
      } else {
        DebugLog(
            "CTRL+R: selected character has no GameData");
      }
    } else {
      DebugLog("CTRL+R: no character selected");
    }
  }
  g_ctrlRPressedLast = controlR;
}

// -----------------------------------------------------------------------
// Plugin entry point
// -----------------------------------------------------------------------
__declspec(dllexport) void startPlugin() {
  if (KenshiLib::SUCCESS !=
      KenshiLib::AddHook(KenshiLib::GetRealAddress(&PlayerInterface::updateUT),
                         PlayerInterface_updateUT_hook,
                         &PlayerInterface_updateUT_originalFunction)) {
    ErrorLog("Could not hook PlayerInterface::updateUT");
    return;
  }

  if (KenshiLib::SUCCESS !=
      KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::dailyUpdate),
                         Character_dailyUpdate_hook,
                         &Character_dailyUpdate_originalFunction)) {
    ErrorLog("Could not hook Character::dailyUpdate");
    return;
  }

  if (KenshiLib::SUCCESS !=
      KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::dailyUpdates),
                         GameWorld_dailyUpdates_hook,
                         &GameWorld_dailyUpdates_originalFunction)) {
    ErrorLog("Could not hook GameWorld::dailyUpdates");
    return;
  }
}