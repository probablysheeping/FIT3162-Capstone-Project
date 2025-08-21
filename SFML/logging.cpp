#include "logging.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

std::ostringstream logBuffer;
Logging logger;

/// <summary>
/// Saves log stream to file
/// </summary>
/// <param name="filename"></param>
void saveLogToFile(const std::string& filename) {
    std::ofstream out(filename);
    out << logBuffer.str();
}

/// <summary>
/// Gets current date and time
/// </summary>
/// <returns>Current time in a string</returns>
std::string currentDateTime() {
	// Get current time as time_point
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);

	// Convert to local time
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &t);   // thread-safe on Windows
#else
	localtime_r(&t, &tm);   // thread-safe on Linux/macOS
#endif

	// Format as string: YYYY-MM-DD_HH-MM-SS
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
	return oss.str();
}
