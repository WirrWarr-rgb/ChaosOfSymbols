#include <iostream>
#include <ctime>
#include "Logger.h"

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

/// <summary>
/// Вывод сообщения в дебаг.лог
/// </summary>
void Logger::Log(const std::string& message) {
    if (!isInitialized) {
        Initialize();
    }

    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.flush();
    }
}

std::string Logger::GetTickCount() {
    std::time_t now = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &now);

    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTime);
    return std::string(timeStr);
}

void Logger::Close() {
    if (logFile.is_open()) {
        Log("=== DEBUG LOG ENDED ===");
        logFile.close();
    }
    isInitialized = false;
}