#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace openmedview {

enum class RuleAction {
	Replace,
	Clear,
	Mask,
	Drop,
	RecursiveReplace
};

struct RuleCondition {
	std::optional<std::string> tagName;
	std::optional<std::string> vr;
};

struct AnonymizationRule {
	std::string tagPattern;
	RuleAction action = RuleAction::Replace;
	std::optional<std::string> value;
	bool recursive = false;
	RuleCondition condition;
};

struct RuleSet {
	std::vector<AnonymizationRule> rules;
};

} // namespace openmedview
