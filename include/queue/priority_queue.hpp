// priority_queue.hpp - Thread-Safe Priority Queue for URL Scheduling
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "models/models.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <functional>

namespace docscraper::queue {

// Thread-safe priority queue for WorkItems
// Lower priority value = higher priority (min-heap)
// Used for BFS-like crawling (lower depth = higher priority)
class ThreadSafePriorityQueue {
public:
    ThreadSafePriorityQueue() = default;
    
    // Non-copyable, non-movable (contains mutex)
    ThreadSafePriorityQueue(const ThreadSafePriorityQueue&) = delete;
    ThreadSafePriorityQueue& operator=(const ThreadSafePriorityQueue&) = delete;
    ThreadSafePriorityQueue(ThreadSafePriorityQueue&&) = delete;
    ThreadSafePriorityQueue& operator=(ThreadSafePriorityQueue&&) = delete;
    
    // Add a work item to the queue
    void push(const models::WorkItem& item);
    void push(models::WorkItem&& item);
    
    // Add multiple items at once
    void push_batch(const std::vector<models::WorkItem>& items);
    
    // Remove and return the highest priority item
    // Blocks if empty until an item is available or the queue is closed
    // Returns nullopt if queue is closed
    std::optional<models::WorkItem> pop();
    
    // Try to pop with timeout
    // Returns nullopt if timeout expires or queue is closed
    std::optional<models::WorkItem> try_pop(std::chrono::milliseconds timeout);
    
    // Try to pop without blocking
    // Returns nullopt if queue is empty or closed
    std::optional<models::WorkItem> try_pop_nonblocking();
    
    // Check if the queue is empty
    bool empty() const;
    
    // Get the current size
    size_t size() const;
    
    // Close the queue (unblock all waiting threads)
    // After closing, pop() returns nullopt
    void close();
    
    // Check if queue is closed
    bool is_closed() const;
    
    // Clear all items
    void clear();
    
    // Get stats
    struct Stats {
        size_t total_pushed = 0;
        size_t total_popped = 0;
        size_t current_size = 0;
        int min_depth = INT_MAX;
        int max_depth = 0;
    };
    Stats get_stats() const;
    
private:
    // Comparator for min-heap (lower priority value = higher priority)
    struct CompareWorkItem {
        bool operator()(const models::WorkItem& a, const models::WorkItem& b) const {
            // Return true if 'a' should be below 'b' in the heap
            // We want lower priority values at the top, so return true if a.priority > b.priority
            return a.priority > b.priority;
        }
    };
    
    std::priority_queue<
        models::WorkItem,
        std::vector<models::WorkItem>,
        CompareWorkItem
    > queue_;
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;
    
    // Stats
    size_t total_pushed_ = 0;
    size_t total_popped_ = 0;
    int min_depth_ = INT_MAX;
    int max_depth_ = 0;
};

// Simple blocking queue (FIFO) for general use
template<typename T>
class BlockingQueue {
public:
    BlockingQueue() = default;
    
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
        cv_.notify_one();
    }
    
    void push(T&& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        cv_.notify_one();
    }
    
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || closed_; });
        
        if (closed_ && queue_.empty()) {
            return std::nullopt;
        }
        
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }
    
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
    }
    
    bool is_closed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }
    
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;
};

// Semaphore for concurrency limiting
class Semaphore {
public:
    explicit Semaphore(int count) : count_(count) {}
    
    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return count_ > 0; });
        --count_;
    }
    
    bool try_acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ > 0) {
            --count_;
            return true;
        }
        return false;
    }
    
    bool try_acquire_for(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this] { return count_ > 0; })) {
            return false;
        }
        --count_;
        return true;
    }
    
    void release() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
        cv_.notify_one();
    }
    
    int available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
    
private:
    int count_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

// RAII semaphore guard
class SemaphoreGuard {
public:
    explicit SemaphoreGuard(Semaphore& sem) : sem_(sem), acquired_(true) {
        sem_.acquire();
    }
    
    ~SemaphoreGuard() {
        if (acquired_) {
            sem_.release();
        }
    }
    
    SemaphoreGuard(const SemaphoreGuard&) = delete;
    SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;
    
    void release() {
        if (acquired_) {
            sem_.release();
            acquired_ = false;
        }
    }
    
private:
    Semaphore& sem_;
    bool acquired_;
};

} // namespace docscraper::queue
