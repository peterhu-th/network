#ifndef AUDIORADARCLIENT_MEMORYCACHE_H
#define AUDIORADARCLIENT_MEMORYCACHE_H

#include <unordered_map>
#include <mutex>
#include <string>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace radar::network {
    namespace utils {

        class MemoryCache {
        public:
            // 单例模式获取实例
            static MemoryCache& getInstance() {
                static MemoryCache instance;
                return instance;
            }

            // 存入缓存并设置 TTL (秒)
            void set(const std::string& key, const std::string& value, int ttl_seconds) {
                std::lock_guard<std::mutex> lock(mutex_);
                auto expire_time = std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
                cache_[key] = {value, expire_time};
            }

            // 获取缓存
            // 成功返回 true，并且 out_value 填充值；不存在或已过期返回 false
            bool get(const std::string& key, std::string& out_value) {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = cache_.find(key);
                if (it == cache_.end()) {
                    return false;
                }

                // 如果当前时间超过过期时间，删除缓存数据
                if (std::chrono::steady_clock::now() > it->second.expire_time) {
                    cache_.erase(it);
                    return false;
                }
                out_value = it->second.value;
                return true;
            }

            void remove(const std::string& key) {
                std::lock_guard<std::mutex> lock(mutex_);
                cache_.erase(key);
            }

        private:
            MemoryCache() : running_(true) {
                cleanup_thread_ = std::thread([this]() {
                    while (running_) {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cv_.wait_for(lock, std::chrono::minutes(10), [this] { return !running_.load(); });
                        if (!running_) break;

                        auto now = std::chrono::steady_clock::now();
                        for (auto it = cache_.begin(); it != cache_.end();) {
                            if (now > it->second.expire_time) {
                                it = cache_.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                });
            }

            ~MemoryCache() {
                running_ = false;
                cv_.notify_all();
                if (cleanup_thread_.joinable()) {
                    cleanup_thread_.join();
                }
            }
            MemoryCache(const MemoryCache&) = delete;
            MemoryCache& operator=(const MemoryCache&) = delete;

            struct CacheItem {
                std::string value;
                std::chrono::steady_clock::time_point expire_time;
            };

            std::unordered_map<std::string, CacheItem> cache_;
            std::mutex mutex_;
            std::condition_variable cv_;
            std::atomic<bool> running_{false};
            std::thread cleanup_thread_;
        };
    }
}

#endif