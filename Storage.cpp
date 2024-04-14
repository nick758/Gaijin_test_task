
#include "Storage.h"

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <stdio.h>

Storage::Storage(const std::string& configPath) :
    configPath(configPath), dataChanged(false)
{
    LoadConfig(configPath);

    for (const auto &item: keysValues)
    {
        printf("key: %s; value: %s\n", item.first.c_str(), item.second.c_str());
    }

    saveThread = std::thread(&Storage::SaveThread, this);
}

Storage::~Storage()
{
    stopThread = true;
    saveThread.join();
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

std::string Storage::Read(const std::string& key) const
{
    std::string result;

    try
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        result = keysValues.at(key);
    }
    catch(std::out_of_range)
    {
        // if there is no such key then return empty string
    }
    
    return std::move(result);
}

void Storage::Write(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(dataMutex);

    keysValues[key] = value;

    dataChanged = true;
}

void Storage::SaveThread()
{
    while (!stopThread)
    {
        std::this_thread::sleep_for(SavePeriod);

        boost::property_tree::ptree pt;

        if (dataChanged)
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            for (const auto& item: keysValues)
            {
                pt.put(item.first, item.second);
            }
        }
        else
        {
            continue;
        }
            
        boost::property_tree::ini_parser::write_ini(configPath, pt);
        dataChanged = false;
    }
}
