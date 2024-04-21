
#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class Storage
{
public:
    Storage(const std::string& configPath);
    ~Storage();

    std::string Read(const std::string& key) const;
    void Write(const std::string& key, const std::string& value);
private:
    const std::chrono::seconds SavePeriod = std::chrono::seconds(1);
    
    std::unordered_map<std::string, std::string> keysValues;
    std::string configPath;

    mutable std::mutex dataMutex;
    std::thread saveThread;
    std::atomic_bool dataChanged;
    std::atomic_bool stopThread;

    void LoadConfig(const std::string& filename);
    void SaveConfig(const std::string& filename);
    void SaveThread();
};
