#include "app/ServerApp.h"
#include "log/Logger.h"

#include <cstdint>
#include <exception>
#include <string>

int main(int argc, char** argv)
{
    std::uint16_t port = 7777;
    if (argc >= 2)
    {
        try
        {
            port = static_cast<std::uint16_t>(std::stoi(argv[1]));
        }
        catch (...)
        {
            LOG_WARN("invalid port arg, fallback to 7777");
        }
    }

    try
    {
        gs::ServerApp app(port);
        app.Run();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(std::string("fatal: ") + e.what());
        return 1;
    }
    return 0;
}