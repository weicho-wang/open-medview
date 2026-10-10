#include "OpenMedView/FileScanner.h"

#include <gdcmReader.h>

namespace openmedview {

namespace {

bool IsDicomFile(const std::filesystem::path& filePath)
{
    gdcm::Reader reader;
    reader.SetFileName(filePath.string().c_str());
    return reader.Read();
}

} // namespace

std::vector<std::filesystem::path> CollectInputFiles(const std::filesystem::path& inputPath, bool recursive)
{
    std::vector<std::filesystem::path> files;

    if (!std::filesystem::exists(inputPath)) {
        return files;
    }

    if (std::filesystem::is_regular_file(inputPath)) {
        files.push_back(inputPath);
        return files;
    }

    if (!std::filesystem::is_directory(inputPath)) {
        return files;
    }

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(inputPath)) {
            if (entry.is_regular_file() && IsDicomFile(entry.path())) {
                files.push_back(entry.path());
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(inputPath)) {
            if (entry.is_regular_file() && IsDicomFile(entry.path())) {
                files.push_back(entry.path());
            }
        }
    }

    return files;
}

} // namespace openmedview
