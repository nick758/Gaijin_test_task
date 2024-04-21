
#include "Storage.h"
#include "Server.h"

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <optional>

#include <stdio.h>

namespace po = boost::program_options;

void SignalHandler(int s)
{
    printf("Caught signal %d\n",s);
    exit(1); 

}

void PrintUsage(const po::options_description& desc)
{
    std::cout << "Usage: options_description [options]\n";
    std::cout << desc;
}

int main(int ac, char** av)
{
    std::cout << "Simple test server" << std::endl;
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);

    boost::asio::ip::port_type port;
    std::string configPath = "./config.txt";
    
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("port,p", po::value<boost::asio::ip::port_type>(&port)->required(), "server's port")
            ("config-file,c", po::value<std::string>(&configPath),
             ("path to the config file, default is " + configPath).c_str());

//        po::positional_options_description p;
//        p.add("input-file", -1);

        po::variables_map vm;
        po::store(po::parse_command_line(ac, av, desc), vm);

        if (vm.count("help"))
        {
            PrintUsage(desc);
            return 0;
        }

        po::notify(vm);
    }
    catch (std::exception& e)
    {
        BOOST_LOG_TRIVIAL(trace) << "main: exception";
        std::cout << "Command line parameters: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Command line parameters: Unknown error!\n";
        return 1;
    }


    const int Signals[] = { SIGHUP, SIGINT, SIGTERM };

    for (int sig: Signals)
    {
        if (signal(sig, &SignalHandler) == SIG_ERR)
        {
            std::cerr << "Can't handle signal " << sig << std::endl;
        }
    }

    std::cout << "configPath: " << configPath << std::endl;
    
    std::optional<Storage> storage;
    try
    {
        storage.emplace(configPath);
    }
    catch (std::exception& e)
    {
        std::cout << "Storage creation: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Storage creation: Unknown error!" << std::endl;
        return 1;
    }

    Server server(port, storage.value());

    server.Start();
    
    return 0;
}
