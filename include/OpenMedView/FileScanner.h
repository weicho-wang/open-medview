#pragma once

#include <filesystem>
#include <vector>

namespace openmedview {

std::vector<std::filesystem::path> CollectInputFiles(const std::filesystem::path& inputPath, bool recursive);

} // namespace openmedview
