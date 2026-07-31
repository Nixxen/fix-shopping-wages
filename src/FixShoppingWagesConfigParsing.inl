// -----------------------------------------------------------------------
// FixShoppingWagesConfigParsing.inl. JSON config file parser
// Included inline by FixShoppingWages.cpp
// Based on Job-B-Gone mod's config parser, with modifications for FixShoppingWages
// https://github.com/Emkej/Job-B-Gone/blob/main/src/JobBGoneConfigParsing.inl
// -----------------------------------------------------------------------

static void SkipJsonWhitespace(const std::string &text, size_t *pos)
{
    if (!pos) { return; }

    while (*pos < text.size() && std::isspace(static_cast<unsigned char>(text[*pos])) != 0)
    {
        ++(*pos);
    }
}

static bool IsJsonLiteralTerminator(char c)
{ return std::isspace(static_cast<unsigned char>(c)) != 0 || c == ',' || c == '}' || c == ']'; }

static void SkipUtf8Bom(const std::string &text, size_t *pos)
{
    if (!pos || *pos != 0 || text.size() < 3) { return; }

    const unsigned char b0 = static_cast<unsigned char>(text[0]);
    const unsigned char b1 = static_cast<unsigned char>(text[1]);
    const unsigned char b2 = static_cast<unsigned char>(text[2]);
    if (b0 == 0xEF && b1 == 0xBB && b2 == 0xBF) { *pos = 3; }
}

static bool RecordConfigSyntaxError(ConfigParseDiagnostics *diagnostics, size_t offset)
{
    if (diagnostics)
    {
        diagnostics->syntaxError = true;
        diagnostics->syntaxErrorOffset = offset;
    }
    return false;
}

static bool ParseJsonStringToken(const std::string &text, size_t *pos, std::string *valueOut)
{
    if (!pos || !valueOut) { return false; }

    SkipJsonWhitespace(text, pos);
    if (*pos >= text.size() || text[*pos] != '"') { return false; }

    ++(*pos);
    valueOut->clear();

    while (*pos < text.size())
    {
        const char c = text[*pos];
        if (c == '"')
        {
            ++(*pos);
            return true;
        }

        if (c == '\\')
        {
            ++(*pos);
            if (*pos >= text.size()) { return false; }
            valueOut->push_back(text[*pos]);
            ++(*pos);
            continue;
        }

        valueOut->push_back(c);
        ++(*pos);
    }

    return false;
}

static bool ParseJsonBoolValue(const std::string &text, size_t *pos, bool *valueOut)
{
    if (!pos || !valueOut) { return false; }

    SkipJsonWhitespace(text, pos);

    if (*pos + 4 <= text.size() && text.compare(*pos, 4, "true") == 0)
    {
        const size_t end = *pos + 4;
        if (end == text.size() || IsJsonLiteralTerminator(text[end]))
        {
            *valueOut = true;
            *pos = end;
            return true;
        }
    }

    if (*pos + 5 <= text.size() && text.compare(*pos, 5, "false") == 0)
    {
        const size_t end = *pos + 5;
        if (end == text.size() || IsJsonLiteralTerminator(text[end]))
        {
            *valueOut = false;
            *pos = end;
            return true;
        }
    }

    return false;
}

static bool
ParseJsonUnsignedIntValue(const std::string &text, size_t *pos, int maxValue, int *valueOut, bool *clampedOut)
{
    if (!pos || !valueOut || maxValue < 0) { return false; }

    SkipJsonWhitespace(text, pos);
    size_t cursor = *pos;
    while (cursor < text.size() && std::isdigit(static_cast<unsigned char>(text[cursor])) != 0)
    {
        ++cursor;
    }

    if (cursor == *pos) { return false; }

    if (cursor < text.size() && !IsJsonLiteralTerminator(text[cursor])) { return false; }

    const std::string numberText = text.substr(*pos, cursor - *pos);
    unsigned long parsed = 0;
    try
    {
        parsed = std::stoul(numberText);
    }
    catch (...)
    {
        return false;
    }

    bool clamped = false;
    if (parsed > static_cast<unsigned long>(maxValue))
    {
        parsed = static_cast<unsigned long>(maxValue);
        clamped = true;
    }

    *valueOut = static_cast<int>(parsed);
    if (clampedOut) { *clampedOut = clamped; }
    *pos = cursor;
    return true;
}

static bool SkipJsonValue(const std::string &text, size_t *pos);

static bool SkipJsonObject(const std::string &text, size_t *pos)
{
    if (!pos || *pos >= text.size() || text[*pos] != '{') { return false; }

    ++(*pos);
    SkipJsonWhitespace(text, pos);
    if (*pos < text.size() && text[*pos] == '}')
    {
        ++(*pos);
        return true;
    }

    while (*pos < text.size())
    {
        std::string ignoredKey;
        if (!ParseJsonStringToken(text, pos, &ignoredKey)) { return false; }

        SkipJsonWhitespace(text, pos);
        if (*pos >= text.size() || text[*pos] != ':') { return false; }

        ++(*pos);
        if (!SkipJsonValue(text, pos)) { return false; }

        SkipJsonWhitespace(text, pos);
        if (*pos >= text.size()) { return false; }

        if (text[*pos] == ',')
        {
            ++(*pos);
            continue;
        }

        if (text[*pos] == '}')
        {
            ++(*pos);
            return true;
        }

        return false;
    }

    return false;
}

static bool SkipJsonArray(const std::string &text, size_t *pos)
{
    if (!pos || *pos >= text.size() || text[*pos] != '[') { return false; }

    ++(*pos);
    SkipJsonWhitespace(text, pos);
    if (*pos < text.size() && text[*pos] == ']')
    {
        ++(*pos);
        return true;
    }

    while (*pos < text.size())
    {
        if (!SkipJsonValue(text, pos)) { return false; }

        SkipJsonWhitespace(text, pos);
        if (*pos >= text.size()) { return false; }

        if (text[*pos] == ',')
        {
            ++(*pos);
            continue;
        }

        if (text[*pos] == ']')
        {
            ++(*pos);
            return true;
        }

        return false;
    }

    return false;
}

static bool SkipJsonValue(const std::string &text, size_t *pos)
{
    if (!pos) { return false; }

    SkipJsonWhitespace(text, pos);
    if (*pos >= text.size()) { return false; }

    const char c = text[*pos];
    if (c == '"')
    {
        std::string ignored;
        return ParseJsonStringToken(text, pos, &ignored);
    }

    if (c == '{') { return SkipJsonObject(text, pos); }
    if (c == '[') { return SkipJsonArray(text, pos); }

    if (c == '-' || std::isdigit(static_cast<unsigned char>(c)) != 0)
    {
        size_t cursor = *pos;
        if (text[cursor] == '-') { ++cursor; }

        bool sawDigit = false;
        while (cursor < text.size() && std::isdigit(static_cast<unsigned char>(text[cursor])) != 0)
        {
            sawDigit = true;
            ++cursor;
        }

        if (!sawDigit) { return false; }

        if (cursor < text.size() && text[cursor] == '.')
        {
            ++cursor;
            bool sawFractionDigit = false;
            while (cursor < text.size() && std::isdigit(static_cast<unsigned char>(text[cursor])) != 0)
            {
                sawFractionDigit = true;
                ++cursor;
            }
            if (!sawFractionDigit) { return false; }
        }

        if (cursor < text.size() && (text[cursor] == 'e' || text[cursor] == 'E'))
        {
            ++cursor;
            if (cursor < text.size() && (text[cursor] == '+' || text[cursor] == '-')) { ++cursor; }

            bool sawExponentDigit = false;
            while (cursor < text.size() && std::isdigit(static_cast<unsigned char>(text[cursor])) != 0)
            {
                sawExponentDigit = true;
                ++cursor;
            }
            if (!sawExponentDigit) { return false; }
        }

        *pos = cursor;
        return true;
    }

    if (*pos + 4 <= text.size() && text.compare(*pos, 4, "true") == 0)
    {
        *pos += 4;
        return true;
    }
    if (*pos + 5 <= text.size() && text.compare(*pos, 5, "false") == 0)
    {
        *pos += 5;
        return true;
    }
    if (*pos + 4 <= text.size() && text.compare(*pos, 4, "null") == 0)
    {
        *pos += 4;
        return true;
    }

    return false;
}

static void ResetConfigParseDiagnostics(ConfigParseDiagnostics *diagnostics)
{
    if (!diagnostics) { return; }

    diagnostics->foundEnabled = false;
    diagnostics->invalidEnabled = false;
    diagnostics->foundVerboseDebugLogging = false;
    diagnostics->invalidVerboseDebugLogging = false;
    diagnostics->foundLimitVerboseDebugLogging = false;
    diagnostics->invalidLimitVerboseDebugLogging = false;
    diagnostics->foundDeveloperDebug = false;
    diagnostics->invalidDeveloperDebug = false;
    diagnostics->foundBaseWageFallback = false;
    diagnostics->invalidBaseWageFallback = false;
    diagnostics->clampedBaseWageFallback = false;
    diagnostics->foundBaseWageOverride = false;
    diagnostics->invalidBaseWageOverride = false;
    diagnostics->foundBaseWageOverrideValue = false;
    diagnostics->invalidBaseWageOverrideValue = false;
    diagnostics->clampedBaseWageOverrideValue = false;
    diagnostics->foundMaxSavingsMultiplier = false;
    diagnostics->invalidMaxSavingsMultiplier = false;
    diagnostics->clampedMaxSavingsMultiplier = false;
    diagnostics->syntaxError = false;
    diagnostics->syntaxErrorOffset = 0;
}

static bool ParseConfigJson(const std::string &body, PluginConfig *configOut, ConfigParseDiagnostics *diagnostics)
{
    if (!configOut || !diagnostics) { return false; }

    size_t pos = 0;
    SkipUtf8Bom(body, &pos);
    SkipJsonWhitespace(body, &pos);
    if (pos >= body.size() || body[pos] != '{') { return RecordConfigSyntaxError(diagnostics, pos); }

    ++pos;
    SkipJsonWhitespace(body, &pos);
    if (pos < body.size() && body[pos] == '}')
    {
        ++pos;
        SkipJsonWhitespace(body, &pos);
        if (pos == body.size()) { return true; }
        return RecordConfigSyntaxError(diagnostics, pos);
    }

    while (pos < body.size())
    {
        std::string key;
        if (!ParseJsonStringToken(body, &pos, &key)) { return RecordConfigSyntaxError(diagnostics, pos); }

        SkipJsonWhitespace(body, &pos);
        if (pos >= body.size() || body[pos] != ':') { return RecordConfigSyntaxError(diagnostics, pos); }
        ++pos;

        if (key == "enabled")
        {
            bool parsedBool = false;
            size_t valuePos = pos;
            if (ParseJsonBoolValue(body, &valuePos, &parsedBool))
            {
                diagnostics->foundEnabled = true;
                configOut->enabled = parsedBool;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidEnabled = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else if (key == "verboseDebugLogging")
        {
            bool parsedBool = false;
            size_t valuePos = pos;
            if (ParseJsonBoolValue(body, &valuePos, &parsedBool))
            {
                diagnostics->foundVerboseDebugLogging = true;
                configOut->verboseDebugLogging = parsedBool;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidVerboseDebugLogging = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else if (key == "limitVerboseDebugLogging")
        {
            bool parsedBool = false;
            size_t valuePos = pos;
            if (ParseJsonBoolValue(body, &valuePos, &parsedBool))
            {
                diagnostics->foundLimitVerboseDebugLogging = true;
                configOut->limitVerboseDebugLogging = parsedBool;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidLimitVerboseDebugLogging = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else if (key == "developerDebug")
        {
            bool parsedBool = false;
            size_t valuePos = pos;
            if (ParseJsonBoolValue(body, &valuePos, &parsedBool))
            {
                diagnostics->foundDeveloperDebug = true;
                configOut->developerDebug = parsedBool;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidDeveloperDebug = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else if (key == "baseWageFallback")
        {
            int parsedInt = 0;
            bool clamped = false;
            size_t valuePos = pos;
            if (ParseJsonUnsignedIntValue(body, &valuePos, kMaxBaseWageFallback, &parsedInt, &clamped))
            {
                diagnostics->foundBaseWageFallback = true;
                diagnostics->clampedBaseWageFallback = diagnostics->clampedBaseWageFallback || clamped;
                configOut->baseWageFallback = parsedInt;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidBaseWageFallback = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else if (key == "baseWageOverride")
        {
            bool parsedBool = false;
            size_t valuePos = pos;
            if (ParseJsonBoolValue(body, &valuePos, &parsedBool))
            {
                diagnostics->foundBaseWageOverride = true;
                configOut->baseWageOverride = parsedBool;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidBaseWageOverride = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else if (key == "baseWageOverrideValue")
        {
            int parsedInt = 0;
            bool clamped = false;
            size_t valuePos = pos;
            if (ParseJsonUnsignedIntValue(body, &valuePos, kMaxBaseWageOverrideValue, &parsedInt, &clamped))
            {
                diagnostics->foundBaseWageOverrideValue = true;
                diagnostics->clampedBaseWageOverrideValue = diagnostics->clampedBaseWageOverrideValue || clamped;
                configOut->baseWageOverrideValue = parsedInt;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidBaseWageOverrideValue = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else if (key == "maxSavingsMultiplier")
        {
            int parsedInt = 0;
            bool clamped = false;
            size_t valuePos = pos;
            if (ParseJsonUnsignedIntValue(body, &valuePos, kMaxMaxSavingsMultiplier, &parsedInt, &clamped))
            {
                diagnostics->foundMaxSavingsMultiplier = true;
                diagnostics->clampedMaxSavingsMultiplier = diagnostics->clampedMaxSavingsMultiplier || clamped;
                configOut->maxSavingsMultiplier = parsedInt;
                pos = valuePos;
            }
            else
            {
                diagnostics->invalidMaxSavingsMultiplier = true;
                if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
            }
        }
        else
        {
            if (!SkipJsonValue(body, &pos)) { return RecordConfigSyntaxError(diagnostics, pos); }
        }

        SkipJsonWhitespace(body, &pos);
        if (pos >= body.size()) { return RecordConfigSyntaxError(diagnostics, pos); }

        if (body[pos] == ',')
        {
            ++pos;
            SkipJsonWhitespace(body, &pos);
            continue;
        }

        if (body[pos] == '}')
        {
            ++pos;
            break;
        }

        return RecordConfigSyntaxError(diagnostics, pos);
    }

    SkipJsonWhitespace(body, &pos);
    if (pos != body.size()) { return RecordConfigSyntaxError(diagnostics, pos); }

    return true;
}

static bool
ReadConfigFromFile(const std::string &configPath, PluginConfig *configOut, bool *foundFileOut, bool *needsWriteBackOut)
{
    if (!configOut) { return false; }

    if (foundFileOut) { *foundFileOut = false; }
    if (needsWriteBackOut) { *needsWriteBackOut = false; }

    std::ifstream in(configPath.c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        if (needsWriteBackOut) { *needsWriteBackOut = true; }
        return true;
    }

    if (foundFileOut) { *foundFileOut = true; }

    const std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ConfigParseDiagnostics diagnostics;
    ResetConfigParseDiagnostics(&diagnostics);
    if (!ParseConfigJson(body, configOut, &diagnostics))
    {
        std::stringstream error;
        error << "FixShoppingWages ERROR: mod-config.json parse error near byte offset "
              << diagnostics.syntaxErrorOffset;
        ErrorLog(error.str().c_str());
        return false;
    }

    bool needsWriteBack = false;
    if (!diagnostics.foundEnabled || diagnostics.invalidEnabled)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"enabled\"; using default");
    }
    if (!diagnostics.foundVerboseDebugLogging || diagnostics.invalidVerboseDebugLogging)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"verboseDebugLogging\"; using default");
    }
    if (!diagnostics.foundLimitVerboseDebugLogging || diagnostics.invalidLimitVerboseDebugLogging)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"limitVerboseDebugLogging\"; using default");
    }
    if (!diagnostics.foundDeveloperDebug || diagnostics.invalidDeveloperDebug)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"developerDebug\"; using default");
    }
    if (!diagnostics.foundBaseWageFallback || diagnostics.invalidBaseWageFallback)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"baseWageFallback\"; using default");
    }
    if (diagnostics.clampedBaseWageFallback)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: \"baseWageFallback\" exceeded max; clamped");
    }
    if (!diagnostics.foundBaseWageOverride || diagnostics.invalidBaseWageOverride)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"baseWageOverride\"; using default");
    }
    if (!diagnostics.foundBaseWageOverrideValue || diagnostics.invalidBaseWageOverrideValue)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"baseWageOverrideValue\"; using default");
    }
    if (diagnostics.clampedBaseWageOverrideValue)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: \"baseWageOverrideValue\" exceeded max; clamped");
    }
    if (!diagnostics.foundMaxSavingsMultiplier || diagnostics.invalidMaxSavingsMultiplier)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: invalid/missing key \"maxSavingsMultiplier\"; using default");
    }
    if (diagnostics.clampedMaxSavingsMultiplier)
    {
        needsWriteBack = true;
        ErrorLog("FixShoppingWages WARN: \"maxSavingsMultiplier\" exceeded max; clamped");
    }
    if (needsWriteBackOut) { *needsWriteBackOut = needsWriteBack; }
    return true;
}

static bool SaveConfigToFile(const std::string &configPath, const PluginConfig &config)
{
    std::ofstream out(configPath.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!out) { return false; }

    out << "{\n";
    out << "  \"enabled\": " << (config.enabled ? "true" : "false") << ",\n";
    out << "  \"verboseDebugLogging\": " << (config.verboseDebugLogging ? "true" : "false") << ",\n";
    out << "  \"limitVerboseDebugLogging\": " << (config.limitVerboseDebugLogging ? "true" : "false") << ",\n";
    out << "  \"developerDebug\": " << (config.developerDebug ? "true" : "false") << ",\n";
    out << "  \"baseWageFallback\": " << config.baseWageFallback << ",\n";
    out << "  \"baseWageOverride\": " << (config.baseWageOverride ? "true" : "false") << ",\n";
    out << "  \"baseWageOverrideValue\": " << config.baseWageOverrideValue << ",\n";
    out << "  \"maxSavingsMultiplier\": " << config.maxSavingsMultiplier << "\n";
    out << "}\n";

    return true;
}