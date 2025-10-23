#pragma once
#include <string>
#include <fstream>
#include <iostream>

class Logger {
public:
    static void Initialize(const std::string& filename = "debug.log");
    static void Log(const std::string& message);
    static void Close();

private:
    static std::ofstream logFile;
    static bool isInitialized;
};