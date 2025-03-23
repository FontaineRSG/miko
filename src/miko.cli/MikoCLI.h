//
// Created by cv2 on 3/23/25.
//
#pragma once
#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include "ClientSession.h"
#include <string>

namespace net = boost::asio;

class CliApp {
public:
    CliApp(net::io_context& io, const std::string& host, const std::string& port);
    void run();
    void process_input(const std::string& line);

private:
    void start_reading_stdin();

    net::io_context& io_context_;
    net::posix::stream_descriptor stdin_;
    net::streambuf stdin_buffer_;
    std::unique_ptr<ClientSession> client_session_;
};