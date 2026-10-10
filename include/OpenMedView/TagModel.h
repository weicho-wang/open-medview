#pragma once

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace openmedview {

struct TagKey {
	unsigned short group = 0;
	unsigned short element = 0;

	std::string ToString() const
	{
		std::ostringstream stream;
		stream << '(' << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << group
			   << ',' << std::setw(4) << std::setfill('0') << element << ')';
		return stream.str();
	}
};

struct TagEntry {
	TagKey key;
	std::string name;
	std::string vr;
	std::string vm;
	std::string value;
	int depth = 0;
	bool isSequence = false;
};

struct TagNode {
	TagEntry entry;
	std::vector<TagNode> children;
};

struct DicomDocument {
	std::filesystem::path sourcePath;
	std::string transferSyntax;
	std::vector<TagNode> roots;
};

} // namespace openmedview
