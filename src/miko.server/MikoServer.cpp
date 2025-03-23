//
// Created by cv2 on 3/23/25.
//

#include "MikoServer.hpp"
#include "Session.hpp"
#include <iostream>
#include <random>
#include <sstream>

static std::string generate_random_string(size_t length) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset)-2);
    std::string str;
    for (size_t i = 0; i < length; i++) {
        str += charset[dist(rng)];
    }
    return str;
}

MikoServer::MikoServer(net::io_context& ioc, tcp::endpoint endpoint)
    : acceptor_(ioc, endpoint), socket_(ioc)
{}

void MikoServer::run() {
    do_accept();
}

void MikoServer::do_accept() {
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
        if (!ec) {
            std::make_shared<Session>(std::move(socket_), shared_from_this())->start();
        }
        do_accept();
    });
}

std::string MikoServer::create_room(const std::string& name) {
    std::string room_id = generate_random_string(8);
    std::string room_key = generate_random_string(16);
    std::string room_name = name.empty() ? room_id : name;
    auto room = std::make_shared<Room>(room_id, room_key, room_name);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rooms_.emplace(room_id, room);
    }
    std::cout << "[Room Created] ID: " << room_id << " Name: " << room_name << " Key: " << room_key << std::endl;
    return room_id;
}

bool MikoServer::join_room(const std::string& room_id, const std::string& room_key,
                             std::shared_ptr<Session> session, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(room_id);
    if (it == rooms_.end() || it->second->get_key() != room_key) {
        return false;
    }
    std::cout << "[User Joined] Room: " << room_id << " Nickname: " << nickname << std::endl;
    return true;
}

void MikoServer::send_room_message(const std::string& room_id, const std::string& nickname, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) return;
    std::string full_message = "[" + nickname + "]: " + message;
    it->second->add_message(full_message);
    // Broadcast to sessions that are in the same room.
    for (auto& session : sessions_) {
        if (session->get_room() == room_id) {
            session->send(full_message);
        }
    }
}

const Room* MikoServer::get_room(const std::string& room_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(room_id);
    if (it != rooms_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void MikoServer::add_session(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(session);
}

void MikoServer::remove_session(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session);
}