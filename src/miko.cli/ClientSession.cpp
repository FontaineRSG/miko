//
// Created by cv2 on 3/23/25.
//

#include "ClientSession.h"
#include "aes_encryption.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <sstream>

#include "Base64.h"

// For brevity, assume the use of plain text in control commands.
// In production, you might factor out command parsing.

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

ClientSession::ClientSession(net::io_context& io)
    : io_context_(io),
      ws_(io)
{}

void ClientSession::connect(const std::string& host, const std::string& port) {
    tcp::resolver resolver(io_context_);
    auto results = resolver.resolve(host, port);
    net::async_connect(ws_.next_layer(), results,
        [this, host](boost::system::error_code ec, const tcp::endpoint&) {
            if (ec) {
                std::cerr << "[ClientSession] Connect error: " << ec.message() << std::endl;
                return;
            }
            ws_.async_handshake(host, "/",
                [this](boost::system::error_code ec) {
                    if (ec) {
                        std::cerr << "[ClientSession] Handshake error: " << ec.message() << std::endl;
                        return;
                    }
                    std::cout << "[ClientSession] Connected to server via WebSocket." << std::endl;
                    start_reading();
                });
        });
    ws_.binary(true);
}

void ClientSession::start_reading() {
    ws_.async_read(ws_buffer_,
        [this](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cerr << "[ClientSession] Read error: " << ec.message() << std::endl;
                return;
            }
            std::string msg = beast::buffers_to_string(ws_buffer_.data());
            ws_buffer_.consume(ws_buffer_.size());
            // If the message is a control command, process it.
            if (msg.rfind("/CMD", 0) == 0) {
                std::cout << "\n[Control] " << msg << std::endl;
                process_control_response(msg);
            } else {
                std::cout << "\n" << msg << std::endl;
            }
            //print_prompt();
            start_reading();
        });
}

// Helper: pack a string with a 4-byte length header.
static void pack_string(std::vector<unsigned char>& buffer, const std::string& s) {
    uint32_t len = htonl(static_cast<uint32_t>(s.size()));
    unsigned char len_bytes[4];
    std::memcpy(len_bytes, &len, 4);
    buffer.insert(buffer.end(), len_bytes, len_bytes + 4);
    buffer.insert(buffer.end(), s.begin(), s.end());
}

void ClientSession::process_input(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    if (!token.empty() && token[0] == '/') {
        if (token == "/create-room") {
            // Extract the room name from the remainder of the line.
            std::string room_name;
            std::getline(iss, room_name);
            // Trim leading spaces:
            room_name.erase(room_name.begin(), std::find_if(room_name.begin(), room_name.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            if (room_name.empty()) {
                std::cerr << "[ClientSession] Usage: /create-room <room_name>" << std::endl;
                return;
            }
            // Send the control command with the room name.
            send_control_command("create-room " + room_name);
        } else if (token == "/join-room") {
            std::string room_id, room_key, nick;
            iss >> room_id >> room_key >> nick;
            if (room_id.empty() || room_key.empty() || nick.empty()) {
                std::cerr << "[ClientSession] Usage: /join-room <room_id> <room_key> <nickname>" << std::endl;
                return;
            }
            set_room(room_id, room_key);
            set_nickname(nick);
            send_control_command("join-room " + room_id + " " + room_key + " " + nick);
        } else if (token == "/nick") {
            std::string newnick;
            iss >> newnick;
            if (newnick.empty()) {
                std::cerr << "[ClientSession] Usage: /nick <new_nickname>" << std::endl;
                return;
            }
            set_nickname(newnick);
            send_control_command("nick " + newnick);
        } else {
            std::cerr << "[ClientSession] Unknown command: " << token << std::endl;
        }
    } else {
        // Regular room message.
        if (!get_room().empty() && !get_room_key().empty()) {
            send_room_message(line);
        } else {
            // If not in a room, send as plain text control message.
            send_control_command(line);
        }
    }
}

void ClientSession::send(const std::string& msg) {
    std::cout << msg << "preparing to send" << std::endl;
    ws_.async_write(net::buffer(msg),
        [this](boost::system::error_code ec, std::size_t) {
            if (ec) {
                std::cerr << "[ClientSession] Send error: " << ec.message() << std::endl;
            }
        });
}

void ClientSession::send_control_command(const std::string& command) {
    // Set WebSocket to text mode.
    ws_.binary(false);
    send("/CMD " + command);
}

void ClientSession::send_room_message(const std::string& line) {
    try {
        AESHelper aes(get_room_key());
        std::string encrypted_payload = aes.encrypt(line);
        // Pack the fields.
        std::vector<unsigned char> packet;
        pack_string(packet, get_room());         // Room ID
        pack_string(packet, get_nickname());       // Nickname
        pack_string(packet, encrypted_payload);    // Encrypted payload

        // Set WebSocket to binary mode.
        ws_.binary(true);
        // Send the binary packet.
        ws_.async_write(boost::asio::buffer(packet),
            [this](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "[ClientSession] Send error: " << ec.message() << std::endl;
                }
            });
    } catch (std::exception& e) {
        std::cerr << "[ClientSession] Encryption error: " << e.what() << std::endl;
    }
}

// In ClientSession or CliApp, when processing an incoming control command:
void ClientSession::process_control_response(const std::string& response) {
    // For example, if response starts with "/CMD join-success", then parse it.
    std::istringstream iss(response);
    std::string prefix, subcmd, room_id, room_name;
    iss >> prefix >> subcmd >> room_id >> room_name;
    if (subcmd == "join-success") {
        set_room(room_id, get_room_key()); // get_room_key() is already stored from join command.
        // Also store the room name.
        current_room_name_ = room_name;
        std::cout << "[ClientSession] Joined room " << room_id << " (" << room_name << ")" << std::endl;
    }
}