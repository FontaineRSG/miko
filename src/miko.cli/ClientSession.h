//
// Created by cv2 on 3/23/25.
//

#pragma once
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <boost/asio/streambuf.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class ClientSession {
public:
    ClientSession(net::io_context& io);

    // Connect to the server using the provided host and port.
    void connect(const std::string& host, const std::string& port);

    // Process user input commands and messages.
    void process_input(const std::string& line);

    // Send a message over the WebSocket.
    void send(const std::string& msg);

    void send_control_command(const std::string &command);

    void send_room_message(const std::string &line);

    void process_control_response(const std::string &response);

    // Getters/setters for state.
    const std::string& get_room() const { return current_room_; }
    const std::string& get_room_key() const { return current_room_key_; }
    const std::string& get_nickname() const { return current_nickname_; }
    std::string current_room_name_;

    void set_room(const std::string& room_id, const std::string& room_key) {
        current_room_ = room_id;
        current_room_key_ = room_key;
    }

    void set_nickname(const std::string& nick) {
        current_nickname_ = nick;
    }

    // Start asynchronous reads.
    void start_reading();

private:
    net::io_context& io_context_;
    websocket::stream<tcp::socket> ws_;
    net::streambuf ws_buffer_;

    // Client state.
    std::string current_room_;
    std::string current_room_key_;
    std::string current_nickname_ = "Anonymous";
};