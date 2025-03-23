#include <MikoCLI.h>
#include <iostream>
#include <sstream>
#include <unistd.h> // For STDIN_FILENO
#include <termios.h>

void disable_echo() {
    struct termios tty{};
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void enable_echo() {
    struct termios tty{};
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

CliApp::CliApp(net::io_context& io, const std::string& host, const std::string& port)
    : io_context_(io),
      stdin_(io, ::dup(STDIN_FILENO))
{
    client_session_ = std::make_unique<ClientSession>(io);
    client_session_->connect(host, port);
}

void CliApp::start_reading_stdin() {
    net::async_read_until(stdin_, stdin_buffer_, '\n',
        [this](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                std::istream is(&stdin_buffer_);
                std::string line;
                std::getline(is, line);
                // Process the input.
                process_input(line);

                // After processing input, clear the line:
                std::cout << "\33[2K\r"; // Clear the entire line.

                // If in a room, disable echo so the input is not shown.
                if (!client_session_->current_room_name_.empty()) {

                } else {
                    std::cout << "Your prompt > " << std::flush;
                }
                start_reading_stdin();
            } else {
                std::cerr << "[CliApp] STDIN read error: " << ec.message() << std::endl;
            }
        });
}

void CliApp::process_input(const std::string& line) {
    // Pass the line to the client session to handle command parsing and sending.
    client_session_->process_input(line);
}

void CliApp::run() {
    start_reading_stdin();
}