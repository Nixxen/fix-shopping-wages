# Fix Shopping Wages

A Kenshi RE_Kenshi plugin that gives NPCs a daily wage allowance so they can afford to shop at player stores. Each NPC's wage is determined by their GameData: a fixed wage value, a random range ("money min" / "money max"), or the local Dried Meat price as a universal baseline. If Dried Meat can't be resolved, a configurable fallback value is used. NPCs flagged for poverty by the base game are skipped.

Get it from your preferred modding platform:
- **Steam Workshop**: [Fix Shopping Wages (RE_Kenshi)](https://steamcommunity.com/sharedfiles/filedetails/?id=3775703318)
- **NexusMods**: [TBD:Fix Shopping Wages](https://www.nexusmods.com/kenshi/mods/TBD)

## Features

- **Daily wage injection** - Every NPC receives their configured wage each day, added to their current money up to a cap
- **Three-tier fallback** - Uses the character's "wages" field first, then a random value between "money min" and "money max", then the local Dried Meat price as a universal baseline
- **Configurable fallback** - If Dried Meat can't be priced (modded away, missing GameData), a configurable fallback wage value is used
- **Poverty-aware** - Kenshi has poverty, and NPCs intentionally set to have no income by the base game are left unchanged. Intentionality is determined by a wage of -1, or wage of 0 with money max of -1
- **Configurable override** - Optionally set a static wage value instead of using Dried Meat pricing. Note that this will not override NPCs with a configured wage in their GameData, only those that would otherwise use the universal baseline  wage
- **Max savings cap** - NPCs won't accumulate more than a configurable multiplier of their daily wage
- **Mod Hub integration** - Configure all settings through Emkejs-Mod-Core's in-game options UI

## Building from source

Uses the recommended setup for KenshiLib mods.
Refer to the [KenshiLib README](https://github.com/BFrizzleFoShizzle/KenshiLib/tree/18f75fecb93cfead6029efe0d5fe199d6618bcc9) for instructions on how to set up a KenshiLib mod environment.

Requires the Mod Hub SDK submodule:

```
git submodule update --init --recursive
```

## Requirements (for the mod to work)

- [RE_Kenshi](https://www.nexusmods.com/kenshi/mods/847) v0.34 or later
- Kenshi v1.0.65+

## Shout Outs

- [KenshiLib](https://github.com/BFrizzleFoShizzle/KenshiLib) by BFrizzleFoShizzle - modding framework
- [Emkejs-Mod-Core](https://github.com/Emkej/Emkejs-Mod-Core) by Emkej - Mod Hub and in-game settings UI