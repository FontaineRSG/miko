//
// Created by cv2 on 3/23/25.
//

#pragma once
#include <string>
#include <stdexcept>
#include <openssl/evp.h>
#include <vector>
#include <cstring>

inline std::string base64_encode(const std::string& input) {
    // Calculate the output length.
    int out_len = 4 * ((input.size() + 2) / 3);
    std::string output;
    output.resize(out_len);
    // EVP_EncodeBlock returns the length of the encoded string.
    int actual_len = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&output[0]),
                                     reinterpret_cast<const unsigned char*>(input.data()),
                                     input.size());
    output.resize(actual_len);
    return output;
}

inline std::string base64_decode(const std::string& input) {
    // The output length will be at most 3/4 of the input length.
    const int out_len = input.size() * 3 / 4;
    std::vector<unsigned char> buffer(out_len);
    int actual_len = EVP_DecodeBlock(buffer.data(),
                                     reinterpret_cast<const unsigned char*>(input.data()),
                                     input.size());
    if (actual_len < 0) {
        throw std::runtime_error("Base64 decode error");
    }
    return std::string(reinterpret_cast<char*>(buffer.data()), actual_len);
}