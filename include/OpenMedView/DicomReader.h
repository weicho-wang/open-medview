#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace openmedview {

struct DicomSummary {
	std::size_t metaHeaderElements = 0;
	std::size_t topLevelDataSetElements = 0;
	std::size_t totalDataElements = 0;
	std::size_t sequenceElements = 0;
	std::size_t itemCount = 0;
	std::size_t privateElements = 0;
	std::size_t privateCreatorElements = 0;
	std::size_t emptyElements = 0;
	std::size_t undefinedLengthElements = 0;
	std::size_t pixelDataElements = 0;
	std::size_t uidElements = 0;
	std::size_t personNameElements = 0;
	std::size_t dateElements = 0;
	std::size_t timeElements = 0;
	std::size_t dateTimeElements = 0;
	std::size_t patientGroupElements = 0;
	std::size_t acquisitionGroupElements = 0;
	std::size_t relationshipGroupElements = 0;
	std::size_t imagePresentationGroupElements = 0;
	std::size_t procedureStepGroupElements = 0;
	std::size_t nuclearMedicineGroupElements = 0;
	std::vector<std::string> topLevelSequenceNames;
	std::vector<std::string> topLevelPrivateCreators;
};

bool PrintDicomTags(const std::filesystem::path& filePath, std::ostream& output);
bool PrintDicomSummary(const std::filesystem::path& filePath, std::ostream& output);

} // namespace openmedview
