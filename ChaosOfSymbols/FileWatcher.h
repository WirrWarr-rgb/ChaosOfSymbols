#pragma once
#include <string>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <filesystem>

class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();

    void WatchFile(const std::string& filePath, std::function<void()> callback);
    void Update();
    void Stop();

private:
    struct FileInfo {
        std::filesystem::file_time_type lastWriteTime;
        std::function<void()> callback;
        bool exists;
    };

    std::unordered_map<std::string, FileInfo> m_watchedFiles;
    bool m_running;
};