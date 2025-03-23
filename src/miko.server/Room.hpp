//
// Created by cv2 on 3/23/25.
//

#pragma once
#include <string>
#include <deque>
#include <mutex>

class Room {
public:
    Room(std::string id, std::string key, std::string name)
        : room_id(std::move(id)), room_key(std::move(key)), room_name(std::move(name)) {}

    void add_message(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (history.size() >= max_history) {
            history.pop_front();
        }
        history.push_back(message);
    }

    const std::deque<std::string>& get_history() const { return history; }
    const std::string& get_key() const { return room_key; }
    const std::string& get_id() const { return room_id; }
    const std::string& get_name() const { return room_name; }

private:
    std::string room_id;
    std::string room_key;
    std::string room_name;
    static const size_t max_history = 16384;
    std::deque<std::string> history;
    mutable std::mutex mutex_;
};
