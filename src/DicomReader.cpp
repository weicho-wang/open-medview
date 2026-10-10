#include "OpenMedView/DicomReader.h"

#include <algorithm>
#include <ostream>
#include <sstream>

#include <gdcmDataSet.h>
#include <gdcmDicts.h>
#include <gdcmDictEntry.h>
#include <gdcmFile.h>
#include <gdcmFileMetaInformation.h>
#include <gdcmGlobal.h>
#include <gdcmPrinter.h>
#include <gdcmReader.h>
#include <gdcmSequenceOfItems.h>
#include <gdcmTag.h>

namespace openmedview {

namespace {

std::string FormatTag(const gdcm::Tag& tag)
{
    std::ostringstream stream;
    stream << tag;
    return stream.str();
}

std::string LookupTagName(const gdcm::Tag& tag)
{
    const gdcm::DictEntry& entry = gdcm::Global::GetInstance().GetDicts().GetDictEntry(tag);
    const char* name = entry.GetName();
    if (name && *name) {
        return name;
    }

    return "Unknown";
}

void AppendUnique(std::vector<std::string>& values, const std::string& value)
{
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

void AccumulateDataSetSummary(const gdcm::DataSet& dataSet, DicomSummary& summary, bool isTopLevel)
{
    for (auto iterator = dataSet.Begin(); iterator != dataSet.End(); ++iterator) {
        const gdcm::DataElement& dataElement = *iterator;
        const gdcm::Tag& tag = dataElement.GetTag();
        const uint16_t group = tag.GetGroup();

        ++summary.totalDataElements;

        if (group == 0x0010) {
            ++summary.patientGroupElements;
        } else if (group == 0x0018) {
            ++summary.acquisitionGroupElements;
        } else if (group == 0x0020) {
            ++summary.relationshipGroupElements;
        } else if (group == 0x0028) {
            ++summary.imagePresentationGroupElements;
        } else if (group == 0x0040) {
            ++summary.procedureStepGroupElements;
        } else if (group == 0x0054) {
            ++summary.nuclearMedicineGroupElements;
        }

        if (dataElement.GetVR() == gdcm::VR::UI) {
            ++summary.uidElements;
        } else if (dataElement.GetVR() == gdcm::VR::PN) {
            ++summary.personNameElements;
        } else if (dataElement.GetVR() == gdcm::VR::DA) {
            ++summary.dateElements;
        } else if (dataElement.GetVR() == gdcm::VR::TM) {
            ++summary.timeElements;
        } else if (dataElement.GetVR() == gdcm::VR::DT) {
            ++summary.dateTimeElements;
        }

        if (tag.IsPrivate()) {
            ++summary.privateElements;
            if (tag.IsPrivateCreator()) {
                ++summary.privateCreatorElements;

                if (isTopLevel) {
                    const gdcm::ByteValue* byteValue = dataElement.GetByteValue();
                    if (byteValue && byteValue->GetPointer()) {
                        std::string creator(byteValue->GetPointer(), byteValue->GetLength());
                        while (!creator.empty() && (creator.back() == '\0' || creator.back() == ' ')) {
                            creator.pop_back();
                        }
                        if (!creator.empty()) {
                            AppendUnique(summary.topLevelPrivateCreators, creator);
                        }
                    }
                }
            }
        }

        if (dataElement.IsEmpty()) {
            ++summary.emptyElements;
        }

        if (dataElement.IsUndefinedLength()) {
            ++summary.undefinedLengthElements;
        }

        if (tag == gdcm::Tag(0x7fe0, 0x0010)) {
            ++summary.pixelDataElements;
        }

        if (dataElement.GetVR() != gdcm::VR::SQ) {
            continue;
        }

        const gdcm::SmartPointer<gdcm::SequenceOfItems> sequence = dataElement.GetValueAsSQ();
        if (!sequence) {
            continue;
        }

        ++summary.sequenceElements;
        summary.itemCount += sequence->GetNumberOfItems();

        if (isTopLevel) {
            AppendUnique(summary.topLevelSequenceNames, FormatTag(tag) + " " + LookupTagName(tag));
        }

        for (std::size_t itemIndex = 1; itemIndex <= sequence->GetNumberOfItems(); ++itemIndex) {
            const gdcm::Item& item = sequence->GetItem(itemIndex);
            AccumulateDataSetSummary(item.GetNestedDataSet(), summary, false);
        }
    }
}

} // namespace

bool PrintDicomTags(const std::filesystem::path& filePath, std::ostream& output)
{
    gdcm::Reader reader;
    reader.SetFileName(filePath.string().c_str());

    if (!reader.Read()) {
        output << "Failed to read DICOM file: " << filePath.string() << '\n';
        return false;
    }

    gdcm::Printer printer;
    printer.SetFile(reader.GetFile());
    printer.Print(output);
    return true;
}

bool PrintDicomSummary(const std::filesystem::path& filePath, std::ostream& output)
{
    gdcm::Reader reader;
    reader.SetFileName(filePath.string().c_str());

    if (!reader.Read()) {
        output << "Failed to read DICOM file: " << filePath.string() << '\n';
        return false;
    }

    DicomSummary summary;
    const gdcm::File& file = reader.GetFile();
    summary.metaHeaderElements = file.GetHeader().Size();
    summary.topLevelDataSetElements = file.GetDataSet().Size();
    AccumulateDataSetSummary(file.GetDataSet(), summary, true);

    output << "Summary for: " << filePath.string() << '\n';
    output << "  Meta header elements: " << summary.metaHeaderElements << '\n';
    output << "  Top-level dataset elements: " << summary.topLevelDataSetElements << '\n';
    output << "  Total dataset elements: " << summary.totalDataElements << '\n';
    output << "  Sequence elements: " << summary.sequenceElements << '\n';
    output << "  Sequence items: " << summary.itemCount << '\n';
    output << "  Private elements: " << summary.privateElements << '\n';
    output << "  Private creator elements: " << summary.privateCreatorElements << '\n';
    output << "  Empty elements: " << summary.emptyElements << '\n';
    output << "  Undefined-length elements: " << summary.undefinedLengthElements << '\n';
    output << "  Pixel Data elements: " << summary.pixelDataElements << '\n';
    output << "  UID elements: " << summary.uidElements << '\n';
    output << "  Person Name elements: " << summary.personNameElements << '\n';
    output << "  Date elements: " << summary.dateElements << '\n';
    output << "  Time elements: " << summary.timeElements << '\n';
    output << "  DateTime elements: " << summary.dateTimeElements << '\n';
    output << "  Patient group elements (0010): " << summary.patientGroupElements << '\n';
    output << "  Acquisition group elements (0018): " << summary.acquisitionGroupElements << '\n';
    output << "  Relationship group elements (0020): " << summary.relationshipGroupElements << '\n';
    output << "  Image presentation group elements (0028): " << summary.imagePresentationGroupElements << '\n';
    output << "  Procedure step group elements (0040): " << summary.procedureStepGroupElements << '\n';
    output << "  Nuclear medicine group elements (0054): " << summary.nuclearMedicineGroupElements << '\n';

    if (!summary.topLevelSequenceNames.empty()) {
        output << "  Top-level sequences:\n";
        for (const auto& name : summary.topLevelSequenceNames) {
            output << "    - " << name << '\n';
        }
    }

    if (!summary.topLevelPrivateCreators.empty()) {
        output << "  Top-level private creators:\n";
        for (const auto& creator : summary.topLevelPrivateCreators) {
            output << "    - " << creator << '\n';
        }
    }

    return true;
}

} // namespace openmedview
