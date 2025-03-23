cmake_minimum_required(VERSION 3.10)
project(miko)

# Find Boost (ensure you list the needed components)
find_package(Boost REQUIRED COMPONENTS system thread)
# Find OpenSSL (for miko.service)
find_package(OpenSSL REQUIRED)

# Build miko.service
add_executable(miko.service
        src/miko.server/MikoServer.cpp
        src/miko.server/MikoServer.hpp
        src/miko.server/Room.hpp
        src/miko.server/Session.cpp
        src/miko.server/Session.hpp
        src/miko.server/main.cpp
        src/miko.server/Base64.h
)
target_include_directories(miko.service PRIVATE src/miko.service)
target_link_libraries(miko.service
        Boost::system
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
)


# Build miko.cli
add_executable(miko.cli
        src/miko.cli/MikoCLI.cpp
        src/miko.cli/MikoCLI.h
        src/miko.cli/main.cpp
        src/miko.cli/ClientSession.cpp
        src/miko.cli/ClientSession.h
        src/miko.server/aes_encryption.h
        src/miko.cli/Base64.h
)
target_include_directories(miko.cli PRIVATE src/miko.cli)
target_link_libraries(miko.cli
        Boost::system
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
)