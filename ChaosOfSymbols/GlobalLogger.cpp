#include "GlobalLogger.h"
#include <iostream>
#include <fstream>
#include <memory>

static std::ofstream g_logFile;
static bool g_logFileInitialized = false;

void GlobalLog(const std::string& message) {
    // Вывод в консоль всегда
    std::cout << "[LOG] " << message << std::endl;

    // И в файл если открыт
    if (g_logFileInitialized && g_logFile.is_open()) {
        g_logFile << message << std::endl;
        g_logFile.flush();
    }
}

void InitializeGlobalLogger(const std::string& filename) {
    g_logFile.open(filename, std::ios::out | std::ios::trunc);
    g_logFileInitialized = g_logFile.is_open();
}

void ShutdownGlobalLogger() {
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
}