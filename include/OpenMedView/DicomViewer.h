#pragma once

#include <filesystem>
#include <iosfwd>

namespace openmedview {

bool GenerateDicomPreview(const std::filesystem::path& inputPath,
                          const std::filesystem::path& outputPath,
                          std::ostream& output);

} // namespace openmedview
