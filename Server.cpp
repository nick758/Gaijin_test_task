
#include "Server.h"

#include "Storage.h"

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
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
    port(port), storage(storage)
{
}


Server::~Server()
{
}

void Server::Start()
{
    MainLoop();
}

void Server::MainLoop()
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor
        acceptor(io_context,
                 boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

    std::cout << "TCP Echo Server started. Listening on port " << port << "." << std::endl;

    while (true)
    {
        std::shared_ptr<boost::asio::ip::tcp::socket> socket =
            std::make_shared<boost::asio::ip::tcp::socket>(io_context);
        acceptor.accept(*socket);

        std::cout << "New connection from: " << socket->remote_endpoint() << std::endl;

        std::thread(&Server::HandleClient, this, socket).detach();
    }
}

void Server::HandleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    boost::system::error_code error;

    while(true)
    {
        boost::asio::streambuf buffer;
        // Read data from the client
        boost::asio::read_until(*socket, buffer, CommandEol, error);
        if (error)
        {
            std::cerr << "Error reading data: " << error.message() << std::endl;
            return;
        }

        // Echo the data back to the client
        std::string message = boost::asio::buffer_cast<const char*>(buffer.data());
        std::cout << message;

        boost::trim_right(message);
        message = HandleCommand(message);
        boost::asio::write(*socket, boost::asio::buffer(message), error);
        if (error) {
            std::cerr << "Error writing data: " << error.message() << std::endl;
            return;
        }
    }
}

std::string Server::HandleCommand(const std::string& line)
{
    std::string result;

    if (line.empty() || line[0] != CommandPrefix)
    {
        return std::move(result);
    }

    std::vector<std::string> commandArg;
    boost::split(commandArg, line, boost::is_any_of(CommandDelimiters));

    int i = 0;
    for (auto const& str: commandArg)
    {
        std::cout << "com_arg " << i << ": \"" << str << "\"" << std::endl;
        ++i;
    }

    if (commandArg.size() < 2)
    {
        BOOST_LOG_TRIVIAL(warning) << "There is no argument for a command (" <<  line << "). Command ignored.";
        return std::move(result);
    }
    else if (commandArg.size() > 2)
    {
        BOOST_LOG_TRIVIAL(warning) << "Too much arguments for a command (" <<  line << "). Extra parameters will be ignored.";
    }
    
    auto& command = commandArg[0];
    
    if (command == CommandGet)
    {
        std::cout << "command: \"" << CommandGet << "\"" << std::endl;
        result = storage.Read(commandArg[1]);
    }
    else if (command == CommandSet)
    {
        std::cout << "command: \"" << CommandSet << "\"" << std::endl;

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
    
    return std::move(result);
}
