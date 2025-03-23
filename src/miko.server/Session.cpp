//
// Created by cv2 on 3/23/25.
//

#include "Session.hpp"
#include "MikoServer.hpp"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include "Base64.h"
#include "aes_encryption.h"

static std::string unpack_string(const unsigned char* data, size_t& offset, size_t total_size) {
    if (offset + 4 > total_size)
        throw std::runtime_error("Invalid packet: unable to read field length");
    uint32_t net_len = 0;
    std::memcpy(&net_len, data + offset, 4);
    uint32_t field_len = ntohl(net_len);
    offset += 4;
    if (offset + field_len > total_size)
        throw std::runtime_error("Invalid packet: field length exceeds packet size");
    std::string field(reinterpret_cast<const char*>(data + offset), field_len);
    offset += field_len;
    return field;
}

struct RoomMessage {
    std::string room_id;
    std::string nickname;
    std::string encrypted_payload;
};

static RoomMessage unpack_room_message(const std::vector<unsigned char>& packet) {
    RoomMessage rm;
    size_t offset = 0;
    size_t total_size = packet.size();
    rm.room_id = unpack_string(packet.data(), offset, total_size);
    rm.nickname = unpack_string(packet.data(), offset, total_size);
    rm.encrypted_payload = unpack_string(packet.data(), offset, total_size);
    return rm;
}


Session::Session(tcp::socket socket, std::shared_ptr<MikoServer> server)
    : ws_(std::move(socket)), server_(server)
{}

void Session::start() {
    server_->add_session(shared_from_this());
    ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
        if (!ec) {
            self->ws_.binary(true);
            self->do_read();
        } else {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        }
    });
}

void Session::client_start(const std::string& host) {
    server_->add_session(shared_from_this());
    ws_.async_handshake(host, "/",
        [self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                self->ws_.binary(true);
                self->do_read();
            } else {
                std::cerr << "Handshake error: " << ec.message() << std::endl;
            }
        }
    );
}

void Session::do_read() {
    ws_.async_read(buffer_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (!ec) {
                if (self->ws_.got_text()) {
                    std::string msg = beast::buffers_to_string(self->buffer_.data());
                    self->buffer_.consume(self->buffer_.size());
                    std::cout << "[command received] " << msg << std::endl;
                    self->process_command(msg);
                } else {
                    // Binary message: pass the raw data to a binary processor.
                    auto data = self->buffer_.data();
                    std::size_t size = boost::asio::buffer_size(data);
                    std::vector<unsigned char> bin_msg(size);
                    std::memcpy(bin_msg.data(), boost::asio::buffer_cast<const void*>(data), size);
                    self->buffer_.consume(size);
                    self->process_binary_room_message(bin_msg);
                }
                self->do_read();
            } else {
                self->server_->remove_session(self);
            }
        });
}

void Session::process_binary_room_message(const std::vector<unsigned char>& bin_msg) {
    try {
        RoomMessage rm = unpack_room_message(bin_msg);
        const Room* room = server_->get_room(rm.room_id);
        if (!room) {
            send("/CMD room-message-failure Room not found");
            return;
        }
        AESHelper aes(room->get_key());
        std::string plaintext = aes.decrypt(rm.encrypted_payload);
        server_->send_room_message(rm.room_id, rm.nickname, plaintext);
    } catch (const std::exception& e) {
        send(std::string("/CMD room-message-failure ") + e.what());
    }
}

void Session::process_command(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string prefix, subcmd;
    iss >> prefix >> subcmd;
    if (subcmd == "create-room") {
        // Optionally, the command may include a room name.
        std::string room_name;
        std::getline(iss, room_name);
        // Trim whitespace.
        room_name.erase(room_name.begin(), std::find_if(room_name.begin(), room_name.end(), [](unsigned char ch){ return !std::isspace(ch); }));
        std::string room_id = server_->create_room(room_name);
        // Retrieve the room name from the created room.
        const Room* room = server_->get_room(room_id);
        std::string rname = room ? room->get_name() : room_id;
        send("/CMD room-created " + room_id + " " + rname);
    } else if (subcmd == "join-room") {
        std::string room_id, room_key, nick;
        iss >> room_id >> room_key >> nick;
        if (room_id.empty() || room_key.empty() || nick.empty()) {
            send("/CMD join-failure Invalid parameters");
            return;
        }
        bool success = server_->join_room(room_id, room_key, shared_from_this(), nick);
        if (success) {
            set_nickname(nick);
            join_room(room_id);
            // Retrieve room name.
            const Room* room = server_->get_room(room_id);
            std::string rname = room ? room->get_name() : room_id;
            send("/CMD join-success " + room_id + " " + rname);
        } else {
            send("/CMD join-failure Invalid room or key");
        }
    } else if (subcmd == "nick") {
        std::string newnick;
        iss >> newnick;
        if (newnick.empty()) {
            send("/CMD nick-failure Missing nickname");
            return;
        }
        set_nickname(newnick);
        send("/CMD nick-changed " + newnick);
    } else if (subcmd == "room-message") {
        // Process binary room-message as before.
        // (Assuming binary message handling remains the same as previously described.)
        const std::string text_prefix = "/CMD room-message ";
        if (cmd.compare(0, text_prefix.size(), text_prefix) == 0) {
            std::string binary_payload = cmd.substr(text_prefix.size());
            std::vector<unsigned char> packet(binary_payload.begin(), binary_payload.end());
            try {
                RoomMessage rm = unpack_room_message(packet);
                const Room* room = server_->get_room(rm.room_id);
                if (!room) {
                    send("/CMD room-message-failure Room not found");
                    return;
                }
                AESHelper aes(room->get_key());
                std::string plaintext = aes.decrypt(rm.encrypted_payload);
                server_->send_room_message(rm.room_id, rm.nickname, plaintext);
            } catch (const std::exception& e) {
                send(std::string("/CMD room-message-failure ") + e.what());
            }
        }
    } else {
        std::cerr << "Unknown command: " << subcmd << std::endl;
    }
}

void Session::send(const std::string& msg) {
    ws_.async_write(net::buffer(msg),
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (ec)
                std::cerr << "Send error: " << ec.message() << std::endl;
        }
    );
}