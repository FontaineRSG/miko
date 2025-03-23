//
// Created by cv2 on 3/23/25.
//

#pragma once
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace ws = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

// Forward declaration of ChatServer
class MikoServer;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, std::shared_ptr<MikoServer> server);
    void start();
    void client_start(const std::string& host);
    void do_read();

    void process_binary_room_message(const std::vector<unsigned char> &bin_msg);

    void process_command(const std::string& cmd);
    void send(const std::string& msg);

    // Setters for per-session state.
    void set_nickname(const std::string& nick) { nickname_ = nick; }
    void join_room(const std::string& room_id) { current_room_ = room_id; }
    const std::string& get_nickname() const { return nickname_; }
    const std::string& get_room() const { return current_room_; }

private:
    ws::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::shared_ptr<MikoServer> server_;
    std::string nickname_ = "Anonymous";
    std::string current_room_; // empty if not in any room.
};