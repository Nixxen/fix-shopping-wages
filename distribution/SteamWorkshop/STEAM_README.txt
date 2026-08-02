[h1]Fix Shopping Wages[/h1]

Ever set up a shop counter only to hear "I can't afford that" within days? That's a known issue with Kenshi. Wages never refresh. No more! Fix Shopping Wages gives most NPCs their daily allowance so they keep buying from player shops.

[b]How does it work?[/b]
Each NPC's wage is pulled from their GameData, checked in this order:
[list=1]
[*]A fixed "wages" value on the character
[*]A random amount between the character's "money min" and "money max" (vanilla Kenshi starting-money fields)
[*]The local Dried Meat price, used as a semi-universal baseline wage (Almost everyone should afford a morsel of Dried Meat, even in Kenshi)
[*]If Dried Meat can't be resolved (modded item, missing data), a configurable fallback kicks in
[/list]
NPCs that are intentionally left poor by the base game are skipped and get nothing.
Two additional notes:
[list]
[*] Some NPCs will get money, but now know how to spend them. In these cases, the mod will give them their dues up to their max limit. They will never spend their money, and will likely die rich.
[*] The money distributed for shopping is separate from money you can loot. A character may have tens of thousands of cats in spending money, but none of that is possible to loot. Likewise, if a character have cats for looting, that can not be spent on shopping.
[/list]

[b]Requirements[/b]
[list]
[*][url=https://www.nexusmods.com/kenshi/mods/847]RE_Kenshi[/url] (tested on v0.34)
[*][url=https://www.nexusmods.com/kenshi/mods/1885]Emkejs-Mod-Core[/url] (optional, for Mod Hub in-game settings UI; load before Fix Shopping Wages)
[*]Kenshi v1.0.65+
[/list]

[b]How to Use[/b]
Install and enable. Wage logic runs automatically each in-game day. No buttons, no windows, no configuration required unless you want to tweak values.
For those that do not use Emkejs mod core, the JSON config file is in the mod folder and can be customized (if you mess it up, delete it and it will restore to defaults).

[b]Features[/b]
[list]
[*][b]Daily wage injection[/b] - NPCs receive their wage each day, added to their current money up to a configurable cap
[*][b]Four-tier fallback[/b] - Uses the character's "wages" field first, then min/max money, then dynamic Dried Meat price, then wiki Dried Meat price
[*][b]Configurable fallback[/b] - If Dried Meat can't be priced, a fallback value takes over (default: 78)
[*][b]Configurable override[/b] - Set a static wage value instead of using Dried Meat pricing. Does not override NPCs with a fixed wage in their GameData
[*][b]Poverty-aware[/b] - NPCs intentionally poor by base-game design are skipped (wage of -1, or wage of 0 with money max of -1)
[*][b]Max savings cap[/b] - NPCs won't accumulate more than a configurable multiplier of their daily wage
[*][b]Configure in-game[/b] - Tweak all values through Emkejs-Mod-Core's options UI (without restarting), or edit the JSON config file directly (requires restart)
[/list]

[b]Recommended Mods[/b]
Fix Shopping Wages was designed with [url=https://steamcommunity.com/sharedfiles/filedetails/?id=1581929438]Enhanced Shopping Economy[/url] in mind. That mod adds wages and expanded shopping lists to many NPCs. Using both mods together, NPCs have both wages to spend and things to buy. Fix Shopping Wages works fine without it due to the fallbacks, but more shoppers means more profit (and let's be honest, seeing a squad of travelers purchase half your stock is fun).

[b]Compatibility[/b]
[list]
[*]No special load order required beyond RE_Kenshi
[*]Does not alter any game files
[*]Works with the KEP shopping expansion, though some NPCs will get double wages.
[*]If using Emkejs Mod Core, load it before this mod
[*]Incompatible with mods that trigger the character daily update cycle (the base game leaves it disabled, likely on purpose)
[/list]

[b]Troubleshooting[/b]
[list]
[*]Confirm RE_Kenshi is installed and enabled
[*]If NPCs are not receiving wages, verify the mod is activated in the launcher
[*]NPCs with a wage of -1, or wage of 0 and money max of -1, are meant to have no income. This is not a bug.
[/list]

[b]Shout outs[/b]
[list]
[*][url=https://steamcommunity.com/id/bmanatee]BFrizzleFoShizzle[/url], creator of KenshiLib and RE_Kenshi
[*][url=https://steamcommunity.com/profiles/76561198014968620]Emkej[/url], creator of Emkejs-Mod-Core for Mod Hub and in-game settings UI
[*][url=https://steamcommunity.com/id/matvey_traveller]Matvey Traveller[/url], creator of Enhanced Shopping Economy, which inspired this mod
[/list]
