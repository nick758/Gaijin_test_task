
#include "Storage.h"

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

Storage::Storage(const std::string& configPath) :
    configPath(configPath), dataChanged(false),
    readCount(0), writeCount(0)
{
    LoadConfig(configPath);

    saveThread = std::thread(&Storage::SaveThread, this);
}

Storage::~Storage()
{
    BOOST_LOG_TRIVIAL(trace) << "~Storage";

    stopThread = true;
    saveThread.join();

    if (dataChanged)
    {
        SaveConfig(configPath);
    }
}

void Storage::LoadConfig(const std::string& filename)
{
    boost::property_tree::ptree pt;

    {
        std::ifstream fileStream(filename);
        if (!fileStream)
        {
            BOOST_LOG_TRIVIAL(info) << "File " + filename + " not exists. Continue with empty storage.";
            return;
        }
    }
    
    boost::property_tree::ini_parser::read_ini(filename, pt);

    for (const auto& item: pt)
    {
        keysValues[item.first.data()] = item.second.data();
    }
}

void Storage::SaveConfig(const std::string& filename)
{
    boost::property_tree::ptree pt;

    {
        std::lock_guard<std::mutex> lock(dataMutex);
        for (const auto& item: keysValues)
        {
            pt.put(item.first, item.second);
        }
    }

    boost::property_tree::ini_parser::write_ini(filename, pt);
}

std::string Storage::Read(const std::string& key) const
{
    std::string result;

    try
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        result = keysValues.at(key);
    }
    catch(std::out_of_range&)
    {
        // if there is no such key then return empty string
    }
    ++readCount;
    
    return result;
}

void Storage::Write(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(dataMutex);

    keysValues[key] = value;

    dataChanged = true;
    ++writeCount;
}

void Storage::SaveThread()
{
    while (!stopThread)
    {
        std::this_thread::sleep_for(SavePeriod);

        if (dataChanged)
        {
            SaveConfig(configPath);
        }
    }
}

StorageStatistics Storage::GetStatistics() const
{
    StorageStatistics result {readCount, writeCount};

    return result;
}
