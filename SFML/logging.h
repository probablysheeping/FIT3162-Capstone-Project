#pragma once
#include <string>
#include <sstream>
#include <iostream>

extern std::ostringstream logBuffer;

/// <summary>
/// Created this class using AI
/// </summary>
class Logging {
public:
    template <typename T>
    Logging& operator<<(const T& value) {
        logBuffer << value;
        std::cout << value;
        return *this;
    }

    Logging& operator<<(std::ostream& (*manip)(std::ostream&)) {
        logBuffer << manip;
        std::cout << manip;
        return *this;
    }
};

extern Logging logger;

void saveLogToFile(const std::string& filename);
std::string currentDateTime();
