#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>

namespace radar::network {
    class CryptoUtils {
    public:
        // 生成加盐哈希值。格式为 base64(salt)$base64(hash)
        static std::string hashPassword(const std::string& raw_password);

        // 验证原密码与存储的加盐哈希是否匹配
        static bool verifyPassword(const std::string& raw_password, const std::string& stored_hash);
    };
}

#endif // CRYPTO_UTILS_H
