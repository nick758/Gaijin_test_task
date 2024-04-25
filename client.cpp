
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <iterator>
#include <random>

namespace po = boost::program_options;

void MainLoop(const std::string& server, boost::asio::ip::port_type port);
void SingleIteraction(boost::asio::ip::tcp::socket& socket, const std::string& request);

void PrintUsage(const po::options_description& desc)
{
    std::cout << "Usage: options_description [options]\n";
    std::cout << desc;
}

int main(int ac, char** av)
{
    std::string server;
    boost::asio::ip::port_type port;

    po::options_description desc("Allowed options");

    try {
        desc.add_options()
            ("help,h", "produce help message")
            ("server,s", po::value<std::string>(&server)->required(),"server's address (required)")
            ("port,p", po::value<boost::asio::ip::port_type>(&port)->required(), "server's port (required)");

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
        std::cout << "Command line parameters: " << e.what() << std::endl;
        PrintUsage(desc);
        return 1;
    }
    catch (...) {
        std::cerr << "Command line parameters: Unknown error!\n";
        PrintUsage(desc);
        return 1;
    }

    MainLoop(server, port);

    return 0;
}

namespace
{
    const std::string RequestKeys[] =
    {
        "accent",
        "accident",
        "account",
        "action",
        "admiration",
        "advice",
        "age",
        "agony",
        "agreement",
        "amount",
        "anger",
        "animal",
        "answer",
        "attitude",
        "battery",
        "belief",
        "belongings",
        "bliss",
        "body",
        "book",
        "breath",
        "brother",
        "bus",
        "care",
        "catastrophe",
        "cause",
        "causes",
        "chance",
        "change",
        "chat",
        "child",
        "circumstances",
        "clothes",
        "coat",
        "coffee",
        "color",
        "colors",
        "company",
        "competition",
        "conscience",
        "contribution",
        "cost",
        "country",
        "course",
        "criticism",
        "damage",
        "day",
        "days",
        "deal",
        "decision",
        "denial",
        "density",
        "depression",
        "description",
        "despair",
        "detail",
        "devotion",
        "diet",
        "disappointment",
        "disaster",
        "dismissal",
        "distance",
        "door",
        "draft",
        "dreamer",
        "eater",
        "ecstasy",
        "effect",
        "effort",
        "end",
        "ending",
        "energy",
        "enjoyment",
        "error",
        "essentials",
        "esteem",
        "estimate",
        "exam",
        "excitement",
        "exercise",
        "expectation",
        "explanation",
        "extras",
        "eyesight",
        "face",
        "failure",
        "faith",
        "family",
        "farming",
        "features",
        "feelings",
        "fix",
        "flight",
        "friend",
        "friends",
        "fun",
        "future",
        "game",
        "generation",
        "grip",
        "growth",
        "habit",
        "hair",
        "happiness",
        "hazard",
        "health",
        "history",
        "home",
        "hour",
        "hours",
        "idea",
        "idiot",
        "illness",
        "impression",
        "improvement",
        "influence",
        "information",
        "ingredient",
        "injury",
        "investment",
        "issue",
        "job",
        "joke",
        "joy",
        "knowledge",
        "language",
        "laundry",
        "length",
        "lesson",
        "level",
        "life",
        "limb",
        "loathing",
        "loser",
        "losses",
        "loyalty",
        "luck",
        "madness",
        "market",
        "meal",
        "medicine",
        "memory",
        "message",
        "minority",
        "mistake",
        "money",
        "mood",
        "motive",
        "movie",
        "music",
        "nationality",
        "neighbors",
        "night",
        "nose",
        "number",
        "obedience",
        "obligation",
        "opinion",
        "order",
        "organ",
        "organs",
        "owners",
        "page",
        "pain",
        "party",
        "pay",
        "penalty",
        "person",
        "player",
        "playing-field",
        "pockets",
        "point",
        "policy",
        "population",
        "power",
        "pressure",
        "price",
        "prisoner",
        "problem",
        "profit",
        "promises",
        "proportion",
        "quality",
        "quantity",
        "question",
        "quote",
        "rain",
        "reason",
        "reception",
        "recovery",
        "reform",
        "refreshments",
        "relationship",
        "relative",
        "reminder",
        "reply",
        "resources",
        "result",
        "review",
        "right",
        "rights",
        "riser",
        "road",
        "role",
        "row",
        "sauce",
        "scale",
        "schedule",
        "seat",
        "sensitivity",
        "service",
        "services",
        "shock",
        "silence",
        "sister",
        "skill",
        "sleep",
        "sleeper",
        "smell",
        "snow",
        "socks",
        "speaker",
        "speech",
        "speed",
        "spirit",
        "sprawl",
        "standard",
        "standards",
        "start",
        "street",
        "strength",
        "supporter",
        "surprise",
        "talker",
        "taxes",
        "teaspoon",
        "teeth",
        "temper",
        "tensions",
        "thing",
        "things",
        "thought",
        "time",
        "tolerance",
        "traffic",
        "trouble",
        "tyre",
        "understanding",
        "victim",
        "view",
        "visibility",
        "voyage",
        "wages",
        "way",
        "wealth",
        "weapon",
        "weather",
        "week",
        "wind",
        "wish",
        "words",
        "work",
        "workload",
        "worth",
        "wreck"
    };
    const std::string Commands[] = { "$get", "$set" };

    const size_t NumberOfIterations = 10000;
    const int PollTimeoutMs = 100;
}

std::string RandomString()
{
    const std::string Characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<> distribution(0, Characters.size() - 1);

    std::uniform_int_distribution<> lengthDistribution(1, 15);

    size_t length = lengthDistribution(generator);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
    {
        random_string += Characters[distribution(generator)];
    }

    return random_string;
}

void MainLoop(const std::string& server, boost::asio::ip::port_type port)
{
    boost::asio::io_context io_context;
    
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);

    boost::asio::connect(socket, resolver.resolve(server, std::to_string(port)));    

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> commandDist(0, 99);
    std::uniform_int_distribution<std::mt19937::result_type> keyDist(0, std::size(RequestKeys) - 1);

    for (size_t i = 0; i < NumberOfIterations; ++i)
    {
        size_t cmdIndex = commandDist(rng) > 0 ? 0 : 1;
        const std::string& command = Commands[cmdIndex];
        const std::string& key = RequestKeys[keyDist(rng)];
        std::string cmdLine = command + " " + key;
        if (cmdIndex == 1)
        {
            cmdLine = cmdLine + "=" + RandomString();
        }
        cmdLine = cmdLine + '\n';
        std::cout << "Command line: " << cmdLine;
        SingleIteraction(socket, cmdLine);
    }

    boost::system::error_code ec;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket.close();
}

void SingleIteraction(boost::asio::ip::tcp::socket& socket, const std::string& request)
{
    socket.non_blocking(true);

    boost::asio::write(socket, boost::asio::buffer(request));
    
    boost::asio::streambuf buffer;
    boost::system::error_code error;

    do
    {
    {
        struct pollfd pollFd {};
        pollFd.fd = socket.native_handle();
        pollFd.events = POLLIN;

        if (::poll(&pollFd, 1, PollTimeoutMs) == -1)
        {
            std::cerr << "error while polling: " << errno << std::endl;
        }
        if (!(pollFd.revents & POLLIN))
        {
            break;
        }
    }
        
    boost::asio::read(socket, buffer, boost::asio::transfer_all(), error);
    if (error)
    {
        if (error.value() != boost::system::errc::resource_unavailable_try_again
            && error.value() != boost::asio::error::eof)
        {
            std::cerr << "Error reading data: " << error.message() << std::endl;
            return;
        }
    }
    } while(false);
    
    const char* data = boost::asio::buffer_cast<const char*>(buffer.data());
    std::cout << data << std::endl;
}
