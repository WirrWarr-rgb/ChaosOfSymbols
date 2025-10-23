#include <thread>
#include "FileWatcher.h"
#include "Logger.h"

namespace fs = std::filesystem;

FileWatcher::FileWatcher() : m_running(true) {}

FileWatcher::~FileWatcher() {
    Stop();
}

/// <summary>
/// ���������� ����� ��� ���������� � callback ��� ����������
/// </summary>
void FileWatcher::WatchFile(const std::string& filePath, std::function<void()> callback) {
    fs::path path(filePath);

    FileInfo fileInfo;
    fileInfo.callback = callback;

    if (fs::exists(path)) {
        fileInfo.lastWriteTime = fs::last_write_time(path);
        fileInfo.exists = true;
        Logger::Log("Started watching file: " + filePath);
    }
    else {
        fileInfo.exists = false;
        Logger::Log("WARNING: File does not exist: " + filePath);
    }

    m_watchedFiles[filePath] = fileInfo;
}

/// <summary>
/// ��������� ��� ����������� ����� �� ��������� � ��������� callback'�
/// </summary>
void FileWatcher::Update() {
    for (auto& pair : m_watchedFiles) {
        const std::string& filePath = pair.first;
        FileInfo& fileInfo = pair.second;

        fs::path path(filePath);
        bool currentlyExists = fs::exists(path);

        // ���� ������ ������������� ���������
        if (currentlyExists != fileInfo.exists) {
            fileInfo.exists = currentlyExists;
            if (currentlyExists) {
                Logger::Log("File appeared: " + filePath);
                fileInfo.lastWriteTime = fs::last_write_time(path);
                fileInfo.callback();
            }
            else {
                Logger::Log("File disappeared: " + filePath);
            }
            continue;
        }

        // ���� ���� ����������, ��������� ���������
        if (currentlyExists) {
            auto currentWriteTime = fs::last_write_time(path);
            if (currentWriteTime != fileInfo.lastWriteTime) {
                Logger::Log("File changed: " + filePath);
                fileInfo.lastWriteTime = currentWriteTime;
                fileInfo.callback();
            }
        }
    }
}   

/// <summary>
/// ������������� ������� ���������� �� �������
/// </summary>
void FileWatcher::Stop() {
    m_running = false;
}