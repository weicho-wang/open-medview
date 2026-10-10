#include "OpenMedView/Anonymizer.h"

#include <algorithm>
#include <filesystem>
#include <ios>
#include <stdexcept>
#include <sstream>
#include <utility>

#include <gdcmDataSet.h>
#include <gdcmAnonymizer.h>
#include <gdcmFile.h>
#include <gdcmReader.h>
#include <gdcmSequenceOfItems.h>
#include <gdcmPrinter.h>
#include <gdcmStringFilter.h>
#include <gdcmTag.h>
#include <gdcmWriter.h>

namespace openmedview {

namespace {

std::string Trim(const std::string& value)
{
    const std::size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }

    const std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool ParseTagPattern(const std::string& pattern, gdcm::Tag& tag)
{
    const std::string cleaned = Trim(pattern);
    if (cleaned.size() != 11 || cleaned.front() != '(' || cleaned[5] != ',' || cleaned.back() != ')') {
        return false;
    }

    try {
        const unsigned short group = static_cast<unsigned short>(std::stoul(cleaned.substr(1, 4), nullptr, 16));
        const unsigned short element = static_cast<unsigned short>(std::stoul(cleaned.substr(6, 4), nullptr, 16));
        tag = gdcm::Tag(group, element);
        return true;
    } catch (...) {
        return false;
    }
}

std::string RuleValueOrEmpty(const AnonymizationRule& rule)
{
    return rule.value ? *rule.value : std::string{};
}

std::string FormatTag(const gdcm::Tag& tag)
{
    std::ostringstream stream;
    stream << tag;
    return stream.str();
}

std::string TrimLine(const std::string& value)
{
    const std::size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }

    const std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string DescribeTagLikeInspect(const gdcm::File& file, const gdcm::DataElement& dataElement)
{
    gdcm::DataSet dataSet;
    dataSet.Insert(dataElement);

    gdcm::Printer printer;
    printer.SetFile(file);
    printer.SetColor(false);

    std::ostringstream stream;
    printer.PrintDataSet(dataSet, stream);

    std::istringstream input(stream.str());
    std::string firstLine;
    std::getline(input, firstLine);
    return TrimLine(firstLine);
}

std::string DescribeTagLikeInspect(const gdcm::File& file, const gdcm::Tag& tag)
{
    const gdcm::DataSet& dataSet = file.GetDataSet();
    if (!dataSet.FindDataElement(tag)) {
        return FormatTag(tag);
    }

    return DescribeTagLikeInspect(file, dataSet.GetDataElement(tag));
}

struct PrivateTagStats {
    std::size_t privateElements = 0;
    std::size_t privateCreators = 0;
};

PrivateTagStats CountPrivateTags(const gdcm::DataSet& dataSet)
{
    PrivateTagStats stats;

    for (auto iterator = dataSet.Begin(); iterator != dataSet.End(); ++iterator) {
        const gdcm::DataElement& dataElement = *iterator;
        const gdcm::Tag& tag = dataElement.GetTag();

        if (tag.IsPrivate()) {
            ++stats.privateElements;
            if (tag.IsPrivateCreator()) {
                ++stats.privateCreators;
            }
        }

        if (dataElement.GetVR() != gdcm::VR::SQ) {
            continue;
        }

        const gdcm::SmartPointer<gdcm::SequenceOfItems> sequence = dataElement.GetValueAsSQ();
        if (!sequence) {
            continue;
        }

        for (std::size_t itemIndex = 1; itemIndex <= sequence->GetNumberOfItems(); ++itemIndex) {
            const gdcm::Item& item = sequence->GetItem(itemIndex);
            const PrivateTagStats nestedStats = CountPrivateTags(item.GetNestedDataSet());
            stats.privateElements += nestedStats.privateElements;
            stats.privateCreators += nestedStats.privateCreators;
        }
    }

    return stats;
}

std::string DescribeTagValue(const gdcm::File& file, const gdcm::Tag& tag)
{
    const gdcm::DataSet& dataSet = file.GetDataSet();
    if (!dataSet.FindDataElement(tag)) {
        return "(missing)";
    }

    const gdcm::DataElement& dataElement = dataSet.GetDataElement(tag);
    if (dataElement.IsEmpty()) {
        return "(empty)";
    }

    gdcm::StringFilter filter;
    filter.SetFile(file);

    try {
        const std::string value = filter.ToString(tag);
        return value.empty() ? "(empty)" : value;
    } catch (...) {
        return "(unprintable)";
    }
}

std::string DescribeDataElementValue(const gdcm::File& file, const gdcm::DataElement& dataElement)
{
    if (dataElement.IsEmpty()) {
        return "(empty)";
    }

    if (dataElement.GetTag() == gdcm::Tag(0x7fe0, 0x0010)) {
        return "(pixel data)";
    }

    const gdcm::VR::VRType vr = dataElement.GetVR();
    if (vr == gdcm::VR::OB || vr == gdcm::VR::OW || vr == gdcm::VR::OF || vr == gdcm::VR::UN) {
        return "(binary)";
    }

    gdcm::StringFilter filter;
    filter.SetFile(file);

    try {
        const std::string value = filter.ToString(dataElement);
        return value.empty() ? "(empty)" : value;
    } catch (...) {
        return "(unprintable)";
    }
}

ChangeRecord MakeChangeRecord(const gdcm::File& file,
                              const std::string& path,
                              const gdcm::DataElement& dataElement,
                              const std::string& original,
                              const std::string& anonymized)
{
    return ChangeRecord{
        path,
        DescribeTagLikeInspect(file, dataElement),
        original,
        anonymized,
    };
}

std::string JoinPath(const std::string& prefix, const std::string& suffix)
{
    if (prefix.empty()) {
        return suffix;
    }

    return prefix + "/" + suffix;
}

void CollectRemovedDataSet(const gdcm::File& originalFile,
                           const gdcm::DataSet& dataSet,
                           const std::string& pathPrefix,
                           std::vector<ChangeRecord>& changes);

void CollectChangedDataSet(const gdcm::File& originalFile,
                           const gdcm::DataSet& originalDataSet,
                           const gdcm::File& anonymizedFile,
                           const gdcm::DataSet& anonymizedDataSet,
                           const std::string& pathPrefix,
                           std::vector<ChangeRecord>& changes)
{
    for (auto iterator = originalDataSet.Begin(); iterator != originalDataSet.End(); ++iterator) {
        const gdcm::DataElement& originalElement = *iterator;
        const gdcm::Tag& tag = originalElement.GetTag();
        if (tag.IsPrivate()) {
            continue;
        }
        const std::string label = JoinPath(pathPrefix, FormatTag(tag));

        if (!anonymizedDataSet.FindDataElement(tag)) {
            changes.push_back(MakeChangeRecord(originalFile,
                                               label,
                                               originalElement,
                                               DescribeDataElementValue(originalFile, originalElement),
                                               "(removed)"));

            if (originalElement.GetVR() == gdcm::VR::SQ) {
                const gdcm::SmartPointer<gdcm::SequenceOfItems> originalSequence = originalElement.GetValueAsSQ();
                if (originalSequence) {
                    for (std::size_t itemIndex = 1; itemIndex <= originalSequence->GetNumberOfItems(); ++itemIndex) {
                        const gdcm::Item& originalItem = originalSequence->GetItem(itemIndex);
                        CollectRemovedDataSet(originalFile,
                                              originalItem.GetNestedDataSet(),
                                              label + "[item" + std::to_string(itemIndex) + "]",
                                              changes);
                    }
                }
            }
            continue;
        }

        const gdcm::DataElement& anonymizedElement = anonymizedDataSet.GetDataElement(tag);

        if (originalElement.GetVR() == gdcm::VR::SQ && anonymizedElement.GetVR() == gdcm::VR::SQ) {
            const gdcm::SmartPointer<gdcm::SequenceOfItems> originalSequence = originalElement.GetValueAsSQ();
            const gdcm::SmartPointer<gdcm::SequenceOfItems> anonymizedSequence = anonymizedElement.GetValueAsSQ();

            if (originalSequence && anonymizedSequence) {
                const std::size_t commonItemCount = std::min(originalSequence->GetNumberOfItems(), anonymizedSequence->GetNumberOfItems());

                for (std::size_t itemIndex = 1; itemIndex <= commonItemCount; ++itemIndex) {
                    const gdcm::Item& originalItem = originalSequence->GetItem(itemIndex);
                    const gdcm::Item& anonymizedItem = anonymizedSequence->GetItem(itemIndex);
                    CollectChangedDataSet(originalFile,
                                          originalItem.GetNestedDataSet(),
                                          anonymizedFile,
                                          anonymizedItem.GetNestedDataSet(),
                                          label + "[item" + std::to_string(itemIndex) + "]",
                                          changes);
                }

                if (originalSequence->GetNumberOfItems() > anonymizedSequence->GetNumberOfItems()) {
                    for (std::size_t itemIndex = anonymizedSequence->GetNumberOfItems() + 1; itemIndex <= originalSequence->GetNumberOfItems(); ++itemIndex) {
                        const gdcm::Item& originalItem = originalSequence->GetItem(itemIndex);
                        CollectRemovedDataSet(originalFile,
                                              originalItem.GetNestedDataSet(),
                                              label + "[item" + std::to_string(itemIndex) + "]",
                                              changes);
                    }
                }

                continue;
            }
        }

        const std::string beforeValue = DescribeDataElementValue(originalFile, originalElement);
        const std::string afterValue = DescribeDataElementValue(anonymizedFile, anonymizedElement);
        if (beforeValue != afterValue) {
            changes.push_back(MakeChangeRecord(originalFile, label, originalElement, beforeValue, afterValue));
        }
    }
}

void CollectRemovedDataSet(const gdcm::File& originalFile,
                           const gdcm::DataSet& dataSet,
                           const std::string& pathPrefix,
                           std::vector<ChangeRecord>& changes)
{
    for (auto iterator = dataSet.Begin(); iterator != dataSet.End(); ++iterator) {
        const gdcm::DataElement& dataElement = *iterator;
        const gdcm::Tag& tag = dataElement.GetTag();
        const std::string label = JoinPath(pathPrefix, FormatTag(tag));

        changes.push_back(MakeChangeRecord(originalFile,
                                           label,
                                           dataElement,
                                           DescribeDataElementValue(originalFile, dataElement),
                                           "(removed)"));

        if (dataElement.GetVR() != gdcm::VR::SQ) {
            continue;
        }

        const gdcm::SmartPointer<gdcm::SequenceOfItems> sequence = dataElement.GetValueAsSQ();
        if (!sequence) {
            continue;
        }

        for (std::size_t itemIndex = 1; itemIndex <= sequence->GetNumberOfItems(); ++itemIndex) {
            const gdcm::Item& item = sequence->GetItem(itemIndex);
            CollectRemovedDataSet(originalFile,
                                  item.GetNestedDataSet(),
                                  label + "[item" + std::to_string(itemIndex) + "]",
                                  changes);
        }
    }
}

bool ApplyRule(gdcm::Anonymizer& anonymizer, const AnonymizationRule& rule)
{
    gdcm::Tag tag;
    if (!ParseTagPattern(rule.tagPattern, tag)) {
        return false;
    }

    switch (rule.action) {
    case RuleAction::Replace:
    case RuleAction::Mask:
    case RuleAction::RecursiveReplace:
        return anonymizer.Replace(tag, RuleValueOrEmpty(rule).c_str());
    case RuleAction::Clear:
        return anonymizer.Empty(tag);
    case RuleAction::Drop:
        return anonymizer.Remove(tag);
    }

    return false;
}

} // namespace

AnonymizeResult AnonymizeFile(const std::filesystem::path& inputPath,
                              const std::filesystem::path& outputPath,
                              const RuleSet& ruleSet,
                              bool dryRun)
{
    AnonymizeResult result;

    gdcm::Reader reader;
    reader.SetFileName(inputPath.string().c_str());

    if (!reader.Read()) {
        result.errorMessage = "Failed to read DICOM file.";
        return result;
    }

    gdcm::Anonymizer anonymizer;
    anonymizer.SetFile(reader.GetFile());

    CollectRemovedDataSet(reader.GetFile(), reader.GetFile().GetDataSet(), std::string{}, result.changes);

    if (anonymizer.RemovePrivateTags()) {
        ++result.modifiedCount;
    }

    for (const auto& rule : ruleSet.rules) {
        gdcm::Tag tag;
        if (!ParseTagPattern(rule.tagPattern, tag)) {
            continue;
        }

        const std::string beforeValue = DescribeTagValue(reader.GetFile(), tag);
        if (ApplyRule(anonymizer, rule)) {
            ++result.modifiedCount;

            const std::string afterValue = DescribeTagValue(anonymizer.GetFile(), tag);
            if (beforeValue != afterValue) {
                result.changes.push_back(ChangeRecord{
                    rule.tagPattern,
                    DescribeTagLikeInspect(reader.GetFile(), tag),
                    beforeValue,
                    afterValue,
                });
            }
        }
    }

    CollectChangedDataSet(reader.GetFile(),
                          reader.GetFile().GetDataSet(),
                          anonymizer.GetFile(),
                          anonymizer.GetFile().GetDataSet(),
                          std::string{},
                          result.changes);

    result.modifiedCount = result.changes.size();

    if (dryRun) {
        result.success = true;
        return result;
    }

    const std::filesystem::path parentPath = outputPath.parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    gdcm::Writer writer;
    writer.SetFileName(outputPath.string().c_str());
    writer.SetFile(anonymizer.GetFile());

    if (!writer.Write()) {
        result.errorMessage = "Failed to write anonymized DICOM file.";
        return result;
    }

    result.success = true;
    return result;
}

} // namespace openmedview
