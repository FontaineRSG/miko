//
// Created by cv2 on 3/23/25.
//
#include "MikoServer.hpp"
#include <boost/asio.hpp>
#include <iostream>

using tcp = boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context ioc;
        tcp::endpoint endpoint{boost::asio::ip::make_address("127.0.0.1"), 19774};
        auto server = std::make_shared<MikoServer>(ioc, endpoint);
        server->run();
        ioc.run();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}