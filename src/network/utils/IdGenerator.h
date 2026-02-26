#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include "../../core/Types.h"
#include <cstdint>
#include <mutex>

namespace radar::network {
     // 雪花算法 ID 生成器：生成唯一的 64 位 ID
     // ID 结构：1位符号位(0) + 41位时间戳 + 10位机器ID + 12位序列号
    class IdGenerator {
    public:
        // 单例模式：外部无法手动创建对象，保证一个类仅有一个实例同时存在
        static IdGenerator& instance();
        Result<int64_t> nextId();
        // 禁用拷贝构造函数和赋值运算符，保证 ID 唯一性
        IdGenerator(const IdGenerator&) = delete;
        IdGenerator& operator=(const IdGenerator&) = delete;

    private:
        explicit IdGenerator(int64_t workerId = 1, int64_t datacenterId = 1);
        ~IdGenerator() = default;

        // 获取当前毫秒时间戳
        static int64_t currentTimestamp();
        // 等待直到下一毫秒
        static int64_t waitNextMillis(int64_t lastTimestamp);

        // 保证时间戳对数据中心和机器唯一
        std::mutex m_mutex;
        int64_t m_lastTimestamp = -1;
        // 毫秒内序列号
        int64_t m_sequence = 0;

        // 配置：工作节点和数据中心
        const int64_t m_workerId;
        const int64_t m_datacenterId;

        // 位移配置
        static constexpr int64_t WORKER_ID_BITS = 5;
        static constexpr int64_t DATACENTER_ID_BITS = 5;
        static constexpr int64_t SEQUENCE_BITS = 12;

        static constexpr int64_t MAX_WORKER_ID = (1LL << WORKER_ID_BITS) - 1;
        static constexpr int64_t MAX_DATACENTER_ID = (1LL << DATACENTER_ID_BITS) - 1;

        static constexpr int64_t WORKER_ID_SHIFT = SEQUENCE_BITS;
        static constexpr int64_t DATACENTER_ID_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS;
        static constexpr int64_t TIMESTAMP_LEFT_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS + DATACENTER_ID_BITS;
        static constexpr int64_t SEQUENCE_MASK = (1LL << SEQUENCE_BITS) - 1;

        // 开始时间戳 (2023-01-01)
        static constexpr int64_t TW_EPOCH = 1672531200000L;
    };
}

#endif // ID_GENERATOR_H
