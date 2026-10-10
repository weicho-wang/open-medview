#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace openmedview {

enum class AppMode {
	Help,
	Inspect,
	Anonymize,
	Preview
};

struct CliOptions {
	AppMode mode = AppMode::Help;
	std::optional<std::filesystem::path> inputPath;
	std::optional<std::filesystem::path> outputPath;
	std::optional<std::filesystem::path> rulesPath;
	bool recursive = false;
	bool summary = false;
	bool dryRun = false;
	bool overwrite = false;
};

} // namespace openmedview
