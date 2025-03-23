#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <algorithm>

class AESHelper {
public:
    static constexpr size_t BLOCK_SIZE = 16; // 16 bytes for AES-128

    explicit AESHelper(const std::string& key) {
        if (key.size() != BLOCK_SIZE) {
            throw std::invalid_argument("Key must be 16 bytes (128 bits).");
        }
        m_key.assign(key.begin(), key.end());
    }

    //Encrypt: prepends the IV to the ciphertext so decryption can retrieve it.
    std::string encrypt(const std::string& plaintext) {
        unsigned char iv[BLOCK_SIZE];
        if (!RAND_bytes(iv, BLOCK_SIZE)) {
            throw std::runtime_error("Error generating random IV.");
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Error creating EVP context.");
        }

        if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, m_key.data(), iv)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Error initializing encryption.");
        }

        std::vector<unsigned char> ciphertext(plaintext.size() + BLOCK_SIZE);
        int len = 0;
        int ciphertext_len = 0;
        if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size())) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Error encrypting data.");
        }
        ciphertext_len = len;

        if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Error finalizing encryption.");
        }
        ciphertext_len += len;
        EVP_CIPHER_CTX_free(ctx);

        // Return iv + ciphertext.
        std::string result(reinterpret_cast<char*>(iv), BLOCK_SIZE);
        result.append(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
        return result;
    }

    //Decrypt: extracts the IV from the beginning of the input.
    std::string decrypt(const std::string& cipher_with_iv) {
        if (cipher_with_iv.size() < BLOCK_SIZE) {
            throw std::runtime_error("Ciphertext too short.");
        }
        const auto* iv = reinterpret_cast<const unsigned char*>(cipher_with_iv.data());
        const auto* ciphertext = reinterpret_cast<const unsigned char*>(cipher_with_iv.data() + BLOCK_SIZE);
        int ciphertext_len = cipher_with_iv.size() - BLOCK_SIZE;

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Error creating EVP context.");
        }
        if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, m_key.data(), iv)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Error initializing decryption.");
        }

        std::vector<unsigned char> plaintext(ciphertext_len + BLOCK_SIZE);
        int len = 0;
        int plaintext_len = 0;
        if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Error decrypting data.");
        }
        plaintext_len = len;

        if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Error finalizing decryption.");
        }
        plaintext_len += len;
        EVP_CIPHER_CTX_free(ctx);
        return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
    }

private:
    std::vector<unsigned char> m_key;
};