#include "IdGenerator.h"
#include <stdexcept>

namespace radar::network {
    IdGenerator& IdGenerator::instance() {
        static IdGenerator instance(1, 1); // 默认机器ID = 1, 数据中心ID = 1
        return instance;
    }

    IdGenerator::IdGenerator(int64_t workerId, int64_t datacenterId)
        : m_workerId(workerId), m_datacenterId(datacenterId) {
        if (workerId > MAX_WORKER_ID || workerId < 0) {
            throw std::runtime_error("Worker ID out of range");
        }
        if (datacenterId > MAX_DATACENTER_ID || datacenterId < 0) {
            throw std::runtime_error("Datacenter ID out of range");
        }
    }

    Result<int64_t> IdGenerator::nextId() {
        // 构造时自动上锁，生命周期结束时自动解锁，保证线程安全
        std::lock_guard<std::mutex> lock(m_mutex);
        int64_t timestamp = currentTimestamp();

        if (timestamp < m_lastTimestamp) {
            return Result<int64_t>::error("Clock moved backwards. Refusing to generate id", ErrorCode::InvalidState);
        }

        if (m_lastTimestamp == timestamp) {
            m_sequence = (m_sequence + 1) & SEQUENCE_MASK;
            if (m_sequence == 0) {
                timestamp = waitNextMillis(m_lastTimestamp);
            }
        } else {
            m_sequence = 0;
        }
        m_lastTimestamp = timestamp;

        return Result<int64_t>::ok(((timestamp - TW_EPOCH) << TIMESTAMP_LEFT_SHIFT) |
               (m_datacenterId << DATACENTER_ID_SHIFT) |
               (m_workerId << WORKER_ID_SHIFT) |
               m_sequence);
    }

    int64_t IdGenerator::currentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    int64_t IdGenerator::waitNextMillis(int64_t lastTimestamp) {
        int64_t timestamp = currentTimestamp();
        while (timestamp <= lastTimestamp) {
            timestamp = currentTimestamp();
        }
        return timestamp;
    }
}
