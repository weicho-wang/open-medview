#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace openmedview {

inline std::string BuildTimestampString()
{
	const auto now = std::chrono::system_clock::now();
	const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

	std::tm localTime{};
#if defined(_WIN32)
	localtime_s(&localTime, &nowTime);
#else
	localtime_r(&nowTime, &localTime);
#endif

	std::ostringstream stream;
	stream << std::put_time(&localTime, "%Y%m%d_%H%M%S");
	return stream.str();
}

inline std::filesystem::path BuildLogFilePath(const std::filesystem::path& logDirectory)
{
	return logDirectory / ("OpenMedView_" + BuildTimestampString() + ".log");
}

class TeeStreambuf final : public std::streambuf {
public:
	TeeStreambuf(std::streambuf* first, std::streambuf* second)
		: first_(first)
		, second_(second)
	{
	}

protected:
	int overflow(int ch) override
	{
		if (traits_type::eq_int_type(ch, traits_type::eof())) {
			return sync() == 0 ? traits_type::not_eof(ch) : traits_type::eof();
		}

		const int firstResult = first_ ? first_->sputc(static_cast<char>(ch)) : ch;
		const int secondResult = second_ ? second_->sputc(static_cast<char>(ch)) : ch;

		if (traits_type::eq_int_type(firstResult, traits_type::eof()) || traits_type::eq_int_type(secondResult, traits_type::eof())) {
			return traits_type::eof();
		}

		return ch;
	}

	int sync() override
	{
		const int firstResult = first_ ? first_->pubsync() : 0;
		const int secondResult = second_ ? second_->pubsync() : 0;
		return (firstResult == 0 && secondResult == 0) ? 0 : -1;
	}

private:
	std::streambuf* first_ = nullptr;
	std::streambuf* second_ = nullptr;
};

class LogSink final {
public:
	explicit LogSink(const std::filesystem::path& logDirectory)
		: logFilePath_(BuildLogFilePath(logDirectory))
	{
		std::filesystem::create_directories(logDirectory);
		fileStream_.open(logFilePath_, std::ios::out | std::ios::app);

		if (fileStream_.is_open()) {
			teeStreambuf_ = std::make_unique<TeeStreambuf>(std::cout.rdbuf(), fileStream_.rdbuf());
			teeStream_ = std::make_unique<std::ostream>(teeStreambuf_.get());
			stream_ = teeStream_.get();
		}
	}

	std::ostream& Stream()
	{
		return *stream_;
	}

	const std::filesystem::path& FilePath() const
	{
		return logFilePath_;
	}

	bool HasLogFile() const
	{
		return fileStream_.is_open();
	}

private:
	std::filesystem::path logFilePath_;
	std::ofstream fileStream_;
	std::unique_ptr<TeeStreambuf> teeStreambuf_;
	std::unique_ptr<std::ostream> teeStream_;
	std::ostream* stream_ = &std::cout;
};

} // namespace openmedview
