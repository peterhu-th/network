#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include <cstdint>
#include <mutex>
#include "../../core/Types.h"

namespace radar::network {
    // 雪花算法 ID 生成器
    // ID 结构：1 位符号位 (0) + 41 位时间戳 + 10 位机器 ID + 12 位序列号
    class IdGenerator {
    public:
        static IdGenerator& instance();
        Result<int64_t> nextId();
        IdGenerator(const IdGenerator&) = delete;
        IdGenerator& operator=(const IdGenerator&) = delete;

    private:
        explicit IdGenerator(int64_t workerId = 1, int64_t datacenterId = 1);
        ~IdGenerator() = default;
        static int64_t currentTimestamp();
        static int64_t waitNextMillis(int64_t lastTimestamp);

        std::mutex m_mutex;
        int64_t m_lastTimestamp;
        int64_t m_sequence = 0;         // 毫秒内序列号
        const int64_t m_workerId;       // 工作节点 ID
        const int64_t m_datacenterId;   // 数据中心 ID

        static constexpr int64_t WORKER_ID_BITS = 5;
        static constexpr int64_t DATACENTER_ID_BITS = 5;
        static constexpr int64_t SEQUENCE_BITS = 12;

        static constexpr int64_t MAX_WORKER_ID = (1LL << WORKER_ID_BITS) - 1;
        static constexpr int64_t MAX_DATACENTER_ID = (1LL << DATACENTER_ID_BITS) - 1;

        static constexpr int64_t WORKER_ID_SHIFT = SEQUENCE_BITS;
        static constexpr int64_t DATACENTER_ID_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS;
        static constexpr int64_t TIMESTAMP_LEFT_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS + DATACENTER_ID_BITS;
        static constexpr int64_t SEQUENCE_MASK = (1LL << SEQUENCE_BITS) - 1;

        static constexpr int64_t TW_EPOCH = 1767225600000L; // 2026 年 1 月 1 日
    };
}

#endif