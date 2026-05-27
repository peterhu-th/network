#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <QByteArray>
#include "CryptoUtils.h"

namespace radar::network {

    static constexpr int SALT_LEN = 16;
    static constexpr int HASH_LEN = 32;
    static constexpr int ITERATIONS = 310000;

    std::string CryptoUtils::hashPassword(const std::string& raw_password) {
        unsigned char salt[SALT_LEN];
        if (RAND_bytes(salt, SALT_LEN) != 1) {
            return "";
        }

        unsigned char hash[HASH_LEN];
        if (PKCS5_PBKDF2_HMAC(raw_password.c_str(), static_cast<int>(raw_password.length()),
                              salt, SALT_LEN, ITERATIONS,
                              EVP_sha256(), HASH_LEN, hash) != 1) {
            return "";
        }

        QByteArray b64Salt = QByteArray(reinterpret_cast<char*>(salt), SALT_LEN).toBase64();
        QByteArray b64Hash = QByteArray(reinterpret_cast<char*>(hash), HASH_LEN).toBase64();

        return std::string(b64Salt.constData()) + "$" + std::string(b64Hash.constData());
    }

    bool CryptoUtils::verifyPassword(const std::string& raw_password, const std::string& stored_hash) {
        auto pos = stored_hash.find('$');
        if (pos == std::string::npos) {
            return false;
        }

        std::string b64Salt = stored_hash.substr(0, pos);
        std::string b64Hash = stored_hash.substr(pos + 1);

        QByteArray salt = QByteArray::fromBase64(b64Salt.c_str());
        QByteArray expectedHash = QByteArray::fromBase64(b64Hash.c_str());

        if (salt.length() != SALT_LEN || expectedHash.length() != HASH_LEN) {
            return false;
        }

        unsigned char hash[HASH_LEN];
        if (PKCS5_PBKDF2_HMAC(raw_password.c_str(), static_cast<int>(raw_password.length()),
                              reinterpret_cast<const unsigned char*>(salt.constData()), SALT_LEN, ITERATIONS,
                              EVP_sha256(), HASH_LEN, hash) != 1) {
            return false;
        }

        return CRYPTO_memcmp(expectedHash.constData(), hash, HASH_LEN) == 0;
    }
}
