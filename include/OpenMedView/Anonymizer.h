#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "OpenMedView/RuleModel.h"

namespace openmedview {

struct ChangeRecord {
	std::string path;
	std::string tag;
	std::string original;
	std::string anonymized;
};

struct AnonymizeResult {
	bool success = false;
	std::string errorMessage;
	std::size_t modifiedCount = 0;
	std::vector<ChangeRecord> changes;
};

AnonymizeResult AnonymizeFile(const std::filesystem::path& inputPath,
							  const std::filesystem::path& outputPath,
							  const RuleSet& ruleSet,
							  bool dryRun);

} // namespace openmedview
