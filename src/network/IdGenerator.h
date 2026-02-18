#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include <cstdint>
#include <mutex>
#include <chrono>

namespace radar {
namespace network {

/**
 * @brief 雪花算法 ID 生成器
 * @details 生成唯一的 64 位 ID。
 * 结构：1位符号位(0) + 41位时间戳 + 10位机器ID + 12位序列号
 */
class IdGenerator {
public:
    static IdGenerator& instance();

    /**
     * @brief 生成下一个唯一 ID
     * @return int64_t 唯一 ID
     */
    int64_t nextId();

private:
    IdGenerator(int64_t workerId = 1, int64_t datacenterId = 1);
    ~IdGenerator() = default;
    IdGenerator(const IdGenerator&) = delete;
    IdGenerator& operator=(const IdGenerator&) = delete;

    // 获取当前毫秒时间戳
    int64_t currentTimestamp();
    // 等待直到下一毫秒
    int64_t waitNextMillis(int64_t lastTimestamp);

    std::mutex m_mutex;
    int64_t m_lastTimestamp = -1;
    int64_t m_sequence = 0;

    // 配置
    const int64_t m_workerId;
    const int64_t m_datacenterId;

    // 位移配置
    static const int64_t WORKER_ID_BITS = 5;
    static const int64_t DATACENTER_ID_BITS = 5;
    static const int64_t SEQUENCE_BITS = 12;

    static const int64_t MAX_WORKER_ID = (1LL << WORKER_ID_BITS) - 1;
    static const int64_t MAX_DATACENTER_ID = (1LL << DATACENTER_ID_BITS) - 1;

    static const int64_t WORKER_ID_SHIFT = SEQUENCE_BITS;
    static const int64_t DATACENTER_ID_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS;
    static const int64_t TIMESTAMP_LEFT_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS + DATACENTER_ID_BITS;
    static const int64_t SEQUENCE_MASK = (1LL << SEQUENCE_BITS) - 1;
    
    // 开始时间戳 (2023-01-01)
    static const int64_t TWEPOCH = 1672531200000L;
};

} // namespace network
} // namespace radar

#endif // ID_GENERATOR_H
