
#pragma once

#include <boost/asio.hpp>
#include <list>
#include <memory>
#include <thread>


class Storage;

class Server
{
public:
    Server(const boost::asio::ip::port_type port, Storage& storage);
    ~Server();

    void Start();
private:
    const boost::asio::ip::port_type port;
    const int pollTimeoutMs = 1000;
    const std::chrono::seconds MonitoringSleep = std::chrono::seconds(1);
    Storage& storage;

    std::mutex threadListMutex;
    std::list<std::thread> connectionThreads;

    std::thread mainThread;
    std::thread monitoringThread;

    std::mutex listMutex;
    std::list<std::thread::id> threadsForJoin;

    std::atomic_bool stopConnectionThreads;
    std::atomic_bool stopMonitoringThread;
    std::atomic_bool stopMainThread;

    void MainLoop();
    void HandleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    std::string HandleCommand(const std::string& line);
    void MonitorThreads();
};
