#include "OpenMedView/RuleEngine.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace openmedview {

namespace {

RuleAction ParseAction(const std::string& value)
{
    if (value == "replace") {
        return RuleAction::Replace;
    }

    if (value == "clear") {
        return RuleAction::Clear;
    }

    if (value == "mask") {
        return RuleAction::Mask;
    }

    if (value == "drop") {
        return RuleAction::Drop;
    }

    return RuleAction::RecursiveReplace;
}

} // namespace

std::string ToString(RuleAction action)
{
    switch (action) {
    case RuleAction::Replace:
        return "replace";
    case RuleAction::Clear:
        return "clear";
    case RuleAction::Mask:
        return "mask";
    case RuleAction::Drop:
        return "drop";
    case RuleAction::RecursiveReplace:
        return "recursive_replace";
    }

    return "replace";
}

RuleSet LoadRuleSet(const std::filesystem::path& filePath)
{
    std::ifstream input(filePath);
    if (!input) {
        throw std::runtime_error("Unable to open rule file: " + filePath.string());
    }

    nlohmann::json json;
    input >> json;

    RuleSet ruleSet;
    if (!json.contains("rules") || !json["rules"].is_array()) {
        return ruleSet;
    }

    for (const auto& item : json["rules"]) {
        AnonymizationRule rule;
        rule.tagPattern = item.value("tag", std::string{});
        rule.action = ParseAction(item.value("action", std::string{"replace"}));
        if (item.contains("value") && !item["value"].is_null()) {
            rule.value = item["value"].get<std::string>();
        }
        rule.recursive = item.value("recursive", false);
        ruleSet.rules.push_back(rule);
    }

    return ruleSet;
}

} // namespace openmedview
