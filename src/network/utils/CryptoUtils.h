#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>

namespace radar::network {
    class CryptoUtils {
    public:
        // 加盐哈希：base64(salt)$base64(hash)
        static std::string hashPassword(const std::string& raw_password);
        static bool verifyPassword(const std::string& raw_password, const std::string& stored_hash);
    };
}

#endif
