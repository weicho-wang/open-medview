#pragma once

#include <filesystem>

#include "OpenMedView/RuleModel.h"

namespace openmedview {

RuleSet LoadRuleSet(const std::filesystem::path& filePath);
std::string ToString(RuleAction action);

} // namespace openmedview
