#include "MikoCLI.h"
#include <boost/asio.hpp>
#include <iostream>
#include <string>

namespace net = boost::asio;

int main(int argc, char* argv[]) {
    try {
        std::string host = "127.0.0.1";
        std::string port = "8080";
        // Optionally parse command-line args for host/port.
        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "--host" && i + 1 < argc) {
                host = argv[++i];
            } else if (arg == "--port" && i + 1 < argc) {
                port = argv[++i];
            }
        }
        net::io_context io;
        CliApp app(io, host, port);
        std::cout << "miko.cli v1.0" << std::endl;
        std::cout << "target host: " << host << std::endl;
        std::cout << "target port: " << port << std::endl;
        std::cout << "Your prompt > " << std::flush;
        app.run();
        io.run();
    } catch (std::exception& e) {
        std::cerr << "CLI Exception: " << e.what() << std::endl;
    }
    return 0;
}