#pragma once

#include <string>

struct PluginConfig
{
    bool enabled;
    bool verboseDebugLogging;
    bool limitVerboseDebugLogging;
    bool developerDebug;
    int baseWageFallback;
    bool baseWageOverride;
    int baseWageOverrideValue;
    int maxSavingsMultiplier;
};

struct ConfigParseDiagnostics
{
    bool foundEnabled;
    bool invalidEnabled;
    bool foundVerboseDebugLogging;
    bool invalidVerboseDebugLogging;
    bool foundLimitVerboseDebugLogging;
    bool invalidLimitVerboseDebugLogging;
    bool foundDeveloperDebug;
    bool invalidDeveloperDebug;
    bool foundBaseWageFallback;
    bool invalidBaseWageFallback;
    bool clampedBaseWageFallback;
    bool foundBaseWageOverride;
    bool invalidBaseWageOverride;
    bool foundBaseWageOverrideValue;
    bool invalidBaseWageOverrideValue;
    bool clampedBaseWageOverrideValue;
    bool foundMaxSavingsMultiplier;
    bool invalidMaxSavingsMultiplier;
    bool clampedMaxSavingsMultiplier;
    bool syntaxError;
    size_t syntaxErrorOffset;
};