//
// Created by cv2 on 3/23/25.
//

#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <set>
#include <mutex>
#include "Room.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class Session; // Forward declaration.

class MikoServer : public std::enable_shared_from_this<MikoServer> {
public:
    MikoServer(net::io_context& ioc, tcp::endpoint endpoint);
    void run();

    // Create a room; if 'name' is empty, use room_id as the name.
    std::string create_room(const std::string& name = "");

    // Join room: requires room_id, room_key, and updates the session's state.
    bool join_room(const std::string& room_id, const std::string& room_key,
                   std::shared_ptr<Session> session, const std::string& nickname);

    // Broadcast a room message.
    void send_room_message(const std::string& room_id, const std::string& nickname, const std::string& message);

    const Room* get_room(const std::string& room_id) const;

    void add_session(std::shared_ptr<Session> session);
    void remove_session(std::shared_ptr<Session> session);

private:
    void do_accept();

    tcp::acceptor acceptor_;
    tcp::socket socket_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Room>> rooms_;
    std::set<std::shared_ptr<Session>> sessions_;
};