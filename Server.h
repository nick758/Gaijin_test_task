
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
    Storage& storage;

//    std::list<std::thread>
    void MainLoop();
    void HandleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    std::string HandleCommand(const std::string& line);
};
