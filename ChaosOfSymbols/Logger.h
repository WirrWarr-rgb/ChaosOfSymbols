#pragma once
#include <string>
#include <fstream>
#include <iostream>

class Logger {
private:
    static std::ofstream logFile;
    static bool isInitialized;

public:
    static void Initialize(const std::string& filename = "debug.log");
    static void Log(const std::string& message);
    static void Close();
};