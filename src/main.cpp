#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "OpenMedView/Anonymizer.h"
#include "OpenMedView/AppTypes.h"
#include "OpenMedView/DicomReader.h"
#include "OpenMedView/DicomViewer.h"
#include "OpenMedView/FileScanner.h"
#include "OpenMedView/Logger.h"
#include "OpenMedView/RuleEngine.h"

namespace {

using openmedview::AppMode;
using openmedview::CliOptions;

template <typename T>
void PrintLabeledField(std::ostream& output, const char* label, const T& value)
{
    output << "  " << std::left << std::setw(11) << label << ": " << value << '\n';
}

void PrintChangeTable(const openmedview::AnonymizeResult& anonymizeResult, std::ostream& output)
{
    if (anonymizeResult.changes.empty()) {
        output << "No anonymization value changes recorded.\n";
        return;
    }

    output << "Changes (original -> anonymized):\n";
    for (const auto& change : anonymizeResult.changes) {
        PrintLabeledField(output, "Path", change.path);
        PrintLabeledField(output, "Tag", change.tag);
        PrintLabeledField(output, "Original", std::quoted(change.original));
        PrintLabeledField(output, "Anonymized", std::quoted(change.anonymized));
        output << '\n';
    }
}

void PrintUsage(std::ostream& output)
{
    output << "Usage:\n"
           << "  OpenMedView --inspect --input <path> [--recursive] [--summary]\n"
           << "  OpenMedView --preview --input <file> [--output <file>]\n"
           << "  OpenMedView --anonymize --input <file> [--output <file>] --rules <file> [--dry-run] [--overwrite]\n"
           << "  OpenMedView --help\n";
}

std::filesystem::path BuildDefaultAnonymizedOutputPath(const std::filesystem::path& inputPath)
{
    const std::filesystem::path parentPath = inputPath.parent_path();
    const std::filesystem::path stem = inputPath.stem();
    const std::filesystem::path extension = inputPath.extension().empty() ? ".dcm" : inputPath.extension();
    return parentPath / (stem.string() + "_anon" + extension.string());
}

CliOptions ParseArguments(int argc, char* argv[])
{
    CliOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--help" || argument == "-h") {
            options.mode = AppMode::Help;
            return options;
        }

        if (argument == "--inspect") {
            options.mode = AppMode::Inspect;
            continue;
        }

        if (argument == "--anonymize") {
            options.mode = AppMode::Anonymize;
            continue;
        }

        if (argument == "--preview" || argument == "--browse") {
            options.mode = AppMode::Preview;
            continue;
        }

        if (argument == "--recursive") {
            options.recursive = true;
            continue;
        }

        if (argument == "--summary") {
            options.summary = true;
            continue;
        }

        if (argument == "--dry-run") {
            options.dryRun = true;
            continue;
        }

        if (argument == "--overwrite") {
            options.overwrite = true;
            continue;
        }

        if (argument == "--input" && index + 1 < argc) {
            options.inputPath = std::filesystem::path(argv[++index]);
            continue;
        }

        if (argument == "--output" && index + 1 < argc) {
            options.outputPath = std::filesystem::path(argv[++index]);
            continue;
        }

        if (argument == "--rules" && index + 1 < argc) {
            options.rulesPath = std::filesystem::path(argv[++index]);
            continue;
        }
    }

    return options;
}

void PrintOptions(const CliOptions& options, std::ostream& output)
{
    output << "Mode: ";
    switch (options.mode) {
    case AppMode::Help:
        output << "help";
        break;
    case AppMode::Inspect:
        output << "inspect";
        break;
    case AppMode::Anonymize:
        output << "anonymize";
        break;
    case AppMode::Preview:
        output << "preview";
        break;
    }

    output << '\n';

    if (options.inputPath) {
        output << "Input: " << options.inputPath->string() << '\n';
    }

    if (options.outputPath) {
        output << "Output: " << options.outputPath->string() << '\n';
    }

    if (options.rulesPath) {
        output << "Rules: " << options.rulesPath->string() << '\n';
    }

    output << "Recursive: " << (options.recursive ? "true" : "false") << '\n';
    output << "Summary: " << (options.summary ? "true" : "false") << '\n';
    output << "Dry run: " << (options.dryRun ? "true" : "false") << '\n';
    output << "Overwrite: " << (options.overwrite ? "true" : "false") << '\n';
}

} // namespace

int main(int argc, char* argv[])
{
    const CliOptions options = ParseArguments(argc, argv);
    openmedview::LogSink logSink(std::filesystem::path("log"));
    std::ostream& log = logSink.Stream();

    if (options.mode == AppMode::Help) {
        log << "Open-MedView\n";
        log << "A GDCM-based DICOM image inspection and anonymization CLI.\n\n";
        if (logSink.HasLogFile()) {
            log << "Log file: " << logSink.FilePath().string() << '\n';
        }
        PrintUsage(log);
        return 0;
    }

    log << "Open-MedView\n";
    log << "A GDCM-based DICOM image inspection and anonymization CLI.\n\n";
    if (logSink.HasLogFile()) {
        log << "Log file: " << logSink.FilePath().string() << '\n';
    }
    PrintOptions(options, log);

    if (!options.inputPath) {
        log << "Missing required argument: --input <path>\n";
        PrintUsage(log);
        return 1;
    }

    if (options.mode == AppMode::Anonymize && !options.rulesPath) {
        log << "Anonymize mode requires --rules <file>.\n";
        return 1;
    }

    if (options.mode == AppMode::Anonymize && !std::filesystem::is_regular_file(*options.inputPath)) {
        log << "Anonymize mode currently supports a single input file only.\n";
        return 1;
    }

    if (options.mode == AppMode::Anonymize && options.outputPath && std::filesystem::is_directory(*options.outputPath)) {
        log << "Anonymize mode expects --output to be a file path, not a directory.\n";
        return 1;
    }

    if (options.mode == AppMode::Inspect) {
        log << "Starting inspect processing.\n";
        const std::vector<std::filesystem::path> files = openmedview::CollectInputFiles(*options.inputPath, options.recursive);

        if (files.empty()) {
            log << "No input files found.\n";
            return 1;
        }

        for (const auto& file : files) {
            log << "\n=== " << file.string() << " ===\n";
            log << "Inspecting file.\n";
            const bool success = options.summary
                ? openmedview::PrintDicomSummary(file, log)
                : openmedview::PrintDicomTags(file, log);

            if (!success) {
                log << "Failed to inspect: " << file.string() << '\n';
            }
        }
    } else if (options.mode == AppMode::Preview) {
        log << "Starting DICOM preview generation.\n";

        const std::filesystem::path outputFile = options.outputPath.value_or(
            (*options.inputPath).parent_path() / ((*options.inputPath).stem().string() + "_preview.png"));

        const bool success = openmedview::GenerateDicomPreview(*options.inputPath, outputFile, log);
        if (!success) {
            return 1;
        }
        log << "Preview export complete.\n";
    } else {
        try {
            log << "Loading anonymization rules.\n";
            const openmedview::RuleSet ruleSet = openmedview::LoadRuleSet(*options.rulesPath);

            log << "Loaded " << ruleSet.rules.size() << " anonymization rules.\n";
            log << "Private tags will be removed before rule application.\n";
            for (const auto& rule : ruleSet.rules) {
                log << "  rule: tag=" << rule.tagPattern << ", action=" << openmedview::ToString(rule.action);
                if (rule.value) {
                    log << ", value=" << *rule.value;
                }
                log << ", recursive=" << (rule.recursive ? "true" : "false") << '\n';
            }

            log << "Collecting input files.\n";
            const std::vector<std::filesystem::path> files = openmedview::CollectInputFiles(*options.inputPath, options.recursive);
            if (files.empty()) {
                log << "No input files found.\n";
                return 1;
            }

            std::size_t succeededCount = 0;
            std::size_t failedCount = 0;

            const std::filesystem::path defaultOutputPath = BuildDefaultAnonymizedOutputPath(*options.inputPath);

            for (const auto& file : files) {
                std::filesystem::path outputFile;

                if (options.overwrite) {
                    outputFile = *options.inputPath;
                } else if (options.outputPath) {
                    outputFile = *options.outputPath;
                } else {
                    outputFile = defaultOutputPath;
                }

                log << "Processing file: " << file.string() << '\n';
                log << "Output: " << outputFile.string() << '\n';

                if (outputFile.has_parent_path()) {
                    std::filesystem::create_directories(outputFile.parent_path());
                }

                log << "Applying anonymization rules.\n";
                const openmedview::AnonymizeResult anonymizeResult =
                    openmedview::AnonymizeFile(file, outputFile, ruleSet, options.dryRun);

                if (anonymizeResult.success) {
                    ++succeededCount;
                    log << "Processed: " << file.string() << " (modified " << anonymizeResult.modifiedCount << " tag(s))\n";
                    PrintChangeTable(anonymizeResult, log);
                } else {
                    ++failedCount;
                    log << "Failed: " << file.string() << " - " << anonymizeResult.errorMessage << '\n';
                }
            }

            log << "Summary: succeeded=" << succeededCount << ", failed=" << failedCount << '\n';
        } catch (const std::exception& exception) {
            log << "Failed to load rules: " << exception.what() << '\n';
            return 1;
        }
    }

    if (argc == 1) {
        log << "Use --help to see available options.\n";
    }

    return 0;
}
