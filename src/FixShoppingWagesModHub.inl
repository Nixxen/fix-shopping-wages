// -----------------------------------------------------------------------
// FixShoppingWagesModHub.inl. Emkej's Mod Core (Mod Hub) integration
// Included inline by FixShoppingWages.cpp
// For more information on the Mod Hub SDK, see:
// https://github.com/Emkej/Emkejs-Mod-Core/blob/main/docs/mod-hub-sdk.md
// -----------------------------------------------------------------------

#include "emc/mod_hub_client.h"

#include <sstream>

namespace
{
const char *kHubNamespaceId = "fixes";
const char *kHubNamespaceDisplayName = "Fixes";
const char *kHubModId = "fix_shopping_wages";
const char *kHubModDisplayName = "Fix Shopping Wages";

typedef bool PluginConfig::*ConfigBoolField;
typedef int PluginConfig::*ConfigIntField;

emc::ModHubClient gModHubClient;
bool gModHubClientConfigured = false;

void WriteHubErrorText(char *err_buf, uint32_t err_buf_size, const char *text)
{
    if (err_buf == 0 || err_buf_size == 0u) { return; }

    if (text == 0)
    {
        err_buf[0] = '\0';
        return;
    }

    uint32_t index = 0u;
    while (index + 1u < err_buf_size && text[index] != '\0')
    {
        err_buf[index] = text[index];
        ++index;
    }
    err_buf[index] = '\0';
}

bool IsValidHubUserData(void *user_data) { return user_data == &gModHubClient; }

// -----------------------------------------------------------------------
// Generic get/set helpers (pointer-to-member dispatch)
// -----------------------------------------------------------------------

EMC_Result GetHubBoolSetting(void *user_data, int32_t *out_value, ConfigBoolField field)
{
    if (!IsValidHubUserData(user_data) || out_value == 0) { return EMC_ERR_INVALID_ARGUMENT; }
    *out_value = (gConfig.*field) ? 1 : 0;
    return EMC_OK;
}

EMC_Result
SetHubBoolSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size, ConfigBoolField field)
{
    if (!IsValidHubUserData(user_data))
    {
        WriteHubErrorText(err_buf, err_buf_size, "invalid_user_data");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    if (value != 0 && value != 1)
    {
        WriteHubErrorText(err_buf, err_buf_size, "invalid_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const PluginConfig previous = gConfig;
    PluginConfig updated = previous;
    updated.*field = value != 0;

    gConfig = updated;
    if (!SaveConfigState())
    {
        gConfig = previous;
        WriteHubErrorText(err_buf, err_buf_size, "persist_failed");
        return EMC_ERR_INTERNAL;
    }

    gConfigNeedsWriteBack = false;
    WriteHubErrorText(err_buf, err_buf_size, 0);
    return EMC_OK;
}

EMC_Result GetHubIntSetting(void *user_data, int32_t *out_value, ConfigIntField field)
{
    if (!IsValidHubUserData(user_data) || out_value == 0) { return EMC_ERR_INVALID_ARGUMENT; }
    *out_value = static_cast<int32_t>(gConfig.*field);
    return EMC_OK;
}

EMC_Result SetHubIntSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size, ConfigIntField field)
{
    if (!IsValidHubUserData(user_data))
    {
        WriteHubErrorText(err_buf, err_buf_size, "invalid_user_data");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const PluginConfig previous = gConfig;
    PluginConfig updated = previous;
    updated.*field = value;

    gConfig = updated;
    if (!SaveConfigState())
    {
        gConfig = previous;
        WriteHubErrorText(err_buf, err_buf_size, "persist_failed");
        return EMC_ERR_INTERNAL;
    }

    gConfigNeedsWriteBack = false;
    WriteHubErrorText(err_buf, err_buf_size, 0);
    return EMC_OK;
}

// -----------------------------------------------------------------------
// Per-setting callbacks: bool
// -----------------------------------------------------------------------

EMC_Result __cdecl GetEnabledSetting(void *user_data, int32_t *out_value)
{ return GetHubBoolSetting(user_data, out_value, &PluginConfig::enabled); }

EMC_Result __cdecl SetEnabledSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubBoolSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::enabled); }

EMC_Result __cdecl GetVerboseDebugLoggingSetting(void *user_data, int32_t *out_value)
{ return GetHubBoolSetting(user_data, out_value, &PluginConfig::verboseDebugLogging); }

EMC_Result __cdecl SetVerboseDebugLoggingSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubBoolSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::verboseDebugLogging); }

EMC_Result __cdecl GetLimitVerboseDebugLoggingSetting(void *user_data, int32_t *out_value)
{ return GetHubBoolSetting(user_data, out_value, &PluginConfig::limitVerboseDebugLogging); }

EMC_Result __cdecl
SetLimitVerboseDebugLoggingSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubBoolSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::limitVerboseDebugLogging); }

EMC_Result __cdecl GetDeveloperDebugSetting(void *user_data, int32_t *out_value)
{ return GetHubBoolSetting(user_data, out_value, &PluginConfig::developerDebug); }

EMC_Result __cdecl SetDeveloperDebugSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubBoolSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::developerDebug); }

EMC_Result __cdecl GetBaseWageOverrideSetting(void *user_data, int32_t *out_value)
{ return GetHubBoolSetting(user_data, out_value, &PluginConfig::baseWageOverride); }

EMC_Result __cdecl SetBaseWageOverrideSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubBoolSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::baseWageOverride); }

// -----------------------------------------------------------------------
// Per-setting callbacks: int
// -----------------------------------------------------------------------

EMC_Result __cdecl GetBaseWageFallbackSetting(void *user_data, int32_t *out_value)
{ return GetHubIntSetting(user_data, out_value, &PluginConfig::baseWageFallback); }

EMC_Result __cdecl SetBaseWageFallbackSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubIntSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::baseWageFallback); }

EMC_Result __cdecl GetBaseWageOverrideValueSetting(void *user_data, int32_t *out_value)
{ return GetHubIntSetting(user_data, out_value, &PluginConfig::baseWageOverrideValue); }

EMC_Result __cdecl SetBaseWageOverrideValueSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubIntSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::baseWageOverrideValue); }

EMC_Result __cdecl GetMaxSavingsMultiplierSetting(void *user_data, int32_t *out_value)
{ return GetHubIntSetting(user_data, out_value, &PluginConfig::maxSavingsMultiplier); }

EMC_Result __cdecl SetMaxSavingsMultiplierSetting(void *user_data, int32_t value, char *err_buf, uint32_t err_buf_size)
{ return SetHubIntSetting(user_data, value, err_buf, err_buf_size, &PluginConfig::maxSavingsMultiplier); }

// -----------------------------------------------------------------------
// Client configuration (static const, matching HFR pattern)
// -----------------------------------------------------------------------

void EnsureModHubClientConfigured()
{
    if (gModHubClientConfigured) { return; }

    static const EMC_ModDescriptorV1 kModHubDescriptor = {
        kHubNamespaceId, kHubNamespaceDisplayName, kHubModId, kHubModDisplayName, &gModHubClient
    };

    static const EMC_BoolSettingDefV1 kEnabledSetting = {"enabled",      "Enabled",          "Enable the mod",
                                                         &gModHubClient, &GetEnabledSetting, &SetEnabledSetting};

    static const EMC_BoolSettingDefV1 kVerboseDebugLoggingSetting = {
        "verbose_debug_logging",
        "Verbose debug",
        "Enable excessively verbose diagnostic logging",
        &gModHubClient,
        &GetVerboseDebugLoggingSetting,
        &SetVerboseDebugLoggingSetting
    };

    static const EMC_BoolSettingDefV1 kLimitVerboseDebugLoggingSetting = {
        "limit_verbose_debug_logging",
        "Limit verbose",
        "Skip logging for new/removed characters in verbose debug mode",
        &gModHubClient,
        &GetLimitVerboseDebugLoggingSetting,
        &SetLimitVerboseDebugLoggingSetting
    };

    static const EMC_BoolSettingDefV1 kDeveloperDebugSetting = {
        "developer_debug", "Developer hotkeys",       "Enable CTRL+(SHIFT)+T/R debug hotkeys",
        &gModHubClient,    &GetDeveloperDebugSetting, &SetDeveloperDebugSetting
    };

    static const EMC_BoolSettingDefV1 kBaseWageOverrideSetting = {
        "base_wage_override",
        "Override base wage",
        "Skip Dried Meat local value and use override value (disables fallback)",
        &gModHubClient,
        &GetBaseWageOverrideSetting,
        &SetBaseWageOverrideSetting
    };

    // Condition rules: show override_value only when override is on; hide fallback when override is on
    static const EMC_BoolConditionRuleDefV1 kOverrideConditionRules[] = {
        {"base_wage_override_value", "base_wage_override", EMC_BOOL_CONDITION_EFFECT_HIDE, 0},
        {"base_wage_fallback", "base_wage_override", EMC_BOOL_CONDITION_EFFECT_HIDE, 1}
    };

    static const EMC_IntSettingDefV1 kBaseWageFallbackSetting = {
        "base_wage_fallback",
        "Base wage fallback",
        "Fallback wage if Dried Meat price can't be found (Not used if override is active)",
        &gModHubClient,
        0,
        kMaxBaseWageFallback,
        1,
        &GetBaseWageFallbackSetting,
        &SetBaseWageFallbackSetting
    };

    static const EMC_IntSettingDefV1 kBaseWageOverrideValueSetting = {
        "base_wage_override_value",
        "Override wage value",
        "Static wage used when override is active",
        &gModHubClient,
        0,
        kMaxBaseWageOverrideValue,
        1,
        &GetBaseWageOverrideValueSetting,
        &SetBaseWageOverrideValueSetting
    };

    static const EMC_IntSettingDefV1 kMaxSavingsMultiplierSetting = {
        "max_savings_multiplier",
        "Max savings multiplier",
        "Max days' wages an NPC can accumulate",
        &gModHubClient,
        0,
        kMaxMaxSavingsMultiplier,
        1,
        &GetMaxSavingsMultiplierSetting,
        &SetMaxSavingsMultiplierSetting
    };

    static const char *kSectionDebugId = "advanced";
    static const char *kSectionDebugLabel = "Debug";

    static const emc::ModHubClientSettingRowV1 kModHubRows[] = {
        {emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, "enabled", &kEnabledSetting, nullptr, nullptr},
        {emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, "base_wage_override", &kBaseWageOverrideSetting, nullptr, nullptr},
        {emc::MOD_HUB_CLIENT_SETTING_KIND_INT, "base_wage_override_value", &kBaseWageOverrideValueSetting, nullptr,
         nullptr},
        {emc::MOD_HUB_CLIENT_SETTING_KIND_INT, "base_wage_fallback", &kBaseWageFallbackSetting, nullptr, nullptr},
        {emc::MOD_HUB_CLIENT_SETTING_KIND_INT, "max_savings_multiplier", &kMaxSavingsMultiplierSetting, nullptr,
         nullptr},
        {emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, "developer_debug", &kDeveloperDebugSetting, kSectionDebugId,
         kSectionDebugLabel},
        {emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, "verbose_debug_logging", &kVerboseDebugLoggingSetting, kSectionDebugId,
         kSectionDebugLabel},
        {emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, "limit_verbose_debug_logging", &kLimitVerboseDebugLoggingSetting,
         kSectionDebugId, kSectionDebugLabel}
    };

    static const emc::ModHubClientTableRegistrationV1 kModHubRegistration = {
        &kModHubDescriptor, kModHubRows, static_cast<uint32_t>(sizeof(kModHubRows) / sizeof(kModHubRows[0])),
        kOverrideConditionRules,
        static_cast<uint32_t>(sizeof(kOverrideConditionRules) / sizeof(kOverrideConditionRules[0]))
    };

    emc::ModHubClient::Config config;
    config.table_registration = &kModHubRegistration;
    gModHubClient.SetConfig(config);
    gModHubClientConfigured = true;
}
} // namespace

void FixShoppingWagesModHub_OnStartup()
{
    EnsureModHubClientConfigured();

    const emc::ModHubClient::AttemptResult result = gModHubClient.OnStartup();
    if (result == emc::ModHubClient::ATTACH_SUCCESS)
    {
        DebugLog("INFO: event=mod_hub_attached use_hub_ui=1");
        return;
    }

    if (result == emc::ModHubClient::ATTACH_FAILED)
    {
        if (gModHubClient.IsAttachRetryPending())
        {
            DebugLog("INFO: event=mod_hub_attach_retry_pending use_hub_ui=0");
            return;
        }

        std::stringstream line;
        line << "WARN: event=mod_hub_fallback reason=get_api_failed"
             << " result=" << gModHubClient.LastAttemptFailureResult() << " use_hub_ui=0";
        ErrorLog(line.str().c_str());
        return;
    }

    if (result == emc::ModHubClient::REGISTRATION_FAILED)
    {
        std::stringstream line;
        line << "WARN: event=mod_hub_fallback reason=register_mod_or_setting_failed"
             << " result=" << gModHubClient.LastAttemptFailureResult() << " use_hub_ui=0";
        ErrorLog(line.str().c_str());
        return;
    }

    ErrorLog("WARN: event=mod_hub_fallback reason=invalid_client_configuration use_hub_ui=0");
}
