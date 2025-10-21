#include "Logger.h"
#include <iostream>

std::ofstream Logger::logFile;
bool Logger::isInitialized = false;

void Logger::Initialize(const std::string& filename) {
    if (isInitialized) {
        return;
    }

    logFile.open(filename, std::ios::out | std::ios::trunc);
    if (!logFile.is_open()) {
        std::cerr << "ERROR: Could not open log file: " << filename << std::endl;
        return;
    }

    isInitialized = true;
    Log("=== DEBUG LOG STARTED ===\n");
}

void Logger::Log(const std::string& message) {
    if (!isInitialized) {
        Initialize();
    }

    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.flush();
    }
}

void Logger::Close() {
    if (logFile.is_open()) {
        Log("=== DEBUG LOG ENDED ===");
        logFile.close();
    }
    isInitialized = false;
}