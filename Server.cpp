
#include "Server.h"

#include "Storage.h"

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <boost/system/system_error.hpp>
#include <iostream>

namespace
{
    const char CommandPrefix = '$';
    const char CommandDelimiters[] = " \t\n\r";
    const char KeyValueDelimiter[] = "=";
    const char CommandEol = '\n';
    const std::string CommandGet = std::string(1, CommandPrefix) + "get";
    const std::string CommandSet = std::string(1, CommandPrefix) + "set";
}

Server::Server(const boost::asio::ip::port_type port, Storage& storage) :
    port(port), storage(storage), stopConnectionThreads(false), stopMonitoringThread(false),
    stopMainThread(false)
{
}


Server::~Server()
{
    stopMonitoringThread = true;
    monitoringThread.join();
    BOOST_LOG_TRIVIAL(trace) << "Monitoring thread is joined";            
    
    stopConnectionThreads = true;

    std::lock_guard<std::mutex> lock(threadListMutex);
    for (auto& thIt: connectionThreads)
    {
        thIt.join();
    }
    BOOST_LOG_TRIVIAL(trace) << "Connection threads are joined";            

    stopMainThread = true;
    mainThread.join();
    BOOST_LOG_TRIVIAL(trace) << "Main thread is joined";            
}

void Server::Start()
{
    mainThread = std::thread(&Server::MainLoop, this);
    monitoringThread = std::thread(&Server::MonitorThreads, this);
}

void Server::MainLoop()
{
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::acceptor
        acceptor(ioContext,
                 boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

    BOOST_LOG_TRIVIAL(info) << "TCP Echo Server started. Listening on port " << port << ".";

    while (!stopMainThread)
    {
        std::shared_ptr<boost::asio::ip::tcp::socket> socket =
            std::make_shared<boost::asio::ip::tcp::socket>(ioContext);

        acceptor.non_blocking(true);
        acceptor.listen();

        struct pollfd pollFd {};
        pollFd.fd = acceptor.native_handle();
        pollFd.events = POLLIN;

        if (::poll(&pollFd, 1, pollTimeoutMs) == -1)
        {
            BOOST_LOG_TRIVIAL(warning) << "error while polling: " << errno;            
        }
        if (!(pollFd.revents & POLLIN))
        {
            // No connection. Wait.
            continue;
        }

        acceptor.accept(*socket);

        BOOST_LOG_TRIVIAL(info) << "New connection from: " << socket->remote_endpoint();

        {
            std::lock_guard<std::mutex> lock(threadListMutex);
            connectionThreads.emplace_back(&Server::HandleClient, this, socket);
        }
    }
}

void Server::HandleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    boost::system::error_code error;
    boost::asio::streambuf buffer;

    socket->non_blocking(true);

    while (!stopConnectionThreads)
    {
        if (buffer.size() == 0)
        {
            struct pollfd pollFd {};
            pollFd.fd = socket->native_handle();
            pollFd.events = POLLIN;

            if (::poll(&pollFd, 1, pollTimeoutMs) == -1)
            {
                BOOST_LOG_TRIVIAL(warning) << "error while polling: " << errno;
            }
            if (!(pollFd.revents & POLLIN))
            {
                // No data for reading. Wait.
                continue;
            }
        }
        
        // Read data from the client
        boost::asio::read_until(*socket, buffer, CommandEol, error);
        if (error)
        {
            if (error.value() == boost::system::errc::resource_unavailable_try_again
                || error.value() == boost::asio::error::eof)
            {
                BOOST_LOG_TRIVIAL(trace) << "Reading data: " << error.message();
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "Error reading data: " << error.message();
            }
            break;
        }

        std::string command;
        std::istream is(&buffer);

        std::getline(is, command);
        BOOST_LOG_TRIVIAL(trace) << command;

        boost::trim_right(command);
        std::string message;
        message = HandleCommand(command);
        if (!message.empty())
        {
            boost::asio::write(*socket, boost::asio::buffer(message), error);
            if (error)
            {
                BOOST_LOG_TRIVIAL(error) << "Error writing data: " << error.message() << std::endl;
                break;
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(listMutex);
        threadsForJoin.push_back(std::this_thread::get_id());
    }
}

std::string Server::HandleCommand(const std::string& line)
{
    std::string result;

    if (line.empty() || line[0] != CommandPrefix)
    {
        return result;
    }

    std::vector<std::string> commandArg;
    boost::split(commandArg, line, boost::is_any_of(CommandDelimiters));

    if (commandArg.size() < 2)
    {
        BOOST_LOG_TRIVIAL(warning) << "There is no argument for a command (" <<  line << "). Command ignored.";

        return result;
    }
    else if (commandArg.size() > 2)
    {
        BOOST_LOG_TRIVIAL(warning) << "Too much arguments for a command (" <<  line << "). Extra parameters will be ignored.";
    }
    
    auto& command = commandArg[0];
    
    if (command == CommandGet)
    {
        BOOST_LOG_TRIVIAL(trace) << "command: \"" << CommandGet << "\"" << std::endl;
        result = storage.Read(commandArg[1]);
    }
    else if (command == CommandSet)
    {
        BOOST_LOG_TRIVIAL(trace) << "command: \"" << CommandSet << "\"" << std::endl;

        std::vector<std::string> keyValue;
        boost::split(keyValue, commandArg[1], boost::is_any_of(KeyValueDelimiter));

        if (keyValue.size() == 2)
        {
            storage.Write(keyValue[0], keyValue[1]);
        }
        else
        {
            BOOST_LOG_TRIVIAL(warning) << "invalid key/value pair (" <<  line << "). $set is not perfomed.";
        }
    }
    
    return result;
}

void Server::MonitorThreads()
{
    while (!stopMonitoringThread)
    {
        {
            std::lock_guard<std::mutex> lock1(threadListMutex);
            std::lock_guard<std::mutex> lock2(listMutex);
            if (!(connectionThreads.empty() || threadsForJoin.empty()))
            {
                for (auto thIt = connectionThreads.begin(); thIt != connectionThreads.end(); ++thIt)
                {
                    for (auto jIt = threadsForJoin.begin(); jIt != threadsForJoin.end(); ++jIt)
                    {
                        if (thIt->get_id() == *jIt)
                        {
                            BOOST_LOG_TRIVIAL(trace) << "join thread (" <<  *jIt << ").";
                            thIt->join();
                            thIt = connectionThreads.erase(thIt);
                            jIt = threadsForJoin.erase(jIt);
                        }
                    }
                    if (threadsForJoin.empty())
                    {
                        BOOST_LOG_TRIVIAL(trace) << "Nothing to join.";
                        break;
                    }
                }
            }
        }
        std::this_thread::sleep_for(MonitoringSleep);
    }
}
