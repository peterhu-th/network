#include <chrono>
#include <stdexcept>
#include "IdGenerator.h"

namespace radar::network {
    // 单例模式
    IdGenerator& IdGenerator::instance() {
        static IdGenerator instance(1, 1);
        return instance;
    }

    IdGenerator::IdGenerator(int64_t workerId, int64_t datacenterId)
        : m_lastTimestamp(0), m_workerId(workerId), m_datacenterId(datacenterId) {
        if (workerId > MAX_WORKER_ID || workerId < 0) {
            throw std::runtime_error("worker ID out of range");
        }
        if (datacenterId > MAX_DATACENTER_ID || datacenterId < 0) {
            throw std::runtime_error("Datacenter ID out of range");
        }
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

    Result<int64_t> IdGenerator::nextId() {
        // 自动上锁解锁，保证线程安全
        std::lock_guard<std::mutex> lock(m_mutex);
        int64_t timestamp = currentTimestamp();
        if (timestamp < m_lastTimestamp) {
            return Result<int64_t>::error("Clock moved backwards. Refusing to generate ID", ErrorCode::InvalidState);
        }
        if (timestamp == m_lastTimestamp) {
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
}
