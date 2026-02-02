// priority_queue.cpp - Thread-Safe Priority Queue Implementation
// LLM Documentation Scraper - C++ Implementation

#include "queue/priority_queue.hpp"

namespace docscraper::queue {

void ThreadSafePriorityQueue::push(const models::WorkItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(item);
    ++total_pushed_;
    
    if (item.depth < min_depth_) min_depth_ = item.depth;
    if (item.depth > max_depth_) max_depth_ = item.depth;
    
    cv_.notify_one();
}

void ThreadSafePriorityQueue::push(models::WorkItem&& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    int depth = item.depth;
    queue_.push(std::move(item));
    ++total_pushed_;
    
    if (depth < min_depth_) min_depth_ = depth;
    if (depth > max_depth_) max_depth_ = depth;
    
    cv_.notify_one();
}

void ThreadSafePriorityQueue::push_batch(const std::vector<models::WorkItem>& items) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& item : items) {
        queue_.push(item);
        ++total_pushed_;
        
        if (item.depth < min_depth_) min_depth_ = item.depth;
        if (item.depth > max_depth_) max_depth_ = item.depth;
    }
    cv_.notify_all();
}

std::optional<models::WorkItem> ThreadSafePriorityQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    cv_.wait(lock, [this] { return !queue_.empty() || closed_; });
    
    if (closed_ && queue_.empty()) {
        return std::nullopt;
    }
    
    models::WorkItem item = queue_.top();
    queue_.pop();
    ++total_popped_;
    
    return item;
}

std::optional<models::WorkItem> ThreadSafePriorityQueue::try_pop(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty() || closed_; })) {
        return std::nullopt;  // Timeout
    }
    
    if (closed_ && queue_.empty()) {
        return std::nullopt;
    }
    
    models::WorkItem item = queue_.top();
    queue_.pop();
    ++total_popped_;
    
    return item;
}

std::optional<models::WorkItem> ThreadSafePriorityQueue::try_pop_nonblocking() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return std::nullopt;
    }
    
    models::WorkItem item = queue_.top();
    queue_.pop();
    ++total_popped_;
    
    return item;
}

bool ThreadSafePriorityQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t ThreadSafePriorityQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void ThreadSafePriorityQueue::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    cv_.notify_all();
}

bool ThreadSafePriorityQueue::is_closed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

void ThreadSafePriorityQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}

ThreadSafePriorityQueue::Stats ThreadSafePriorityQueue::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats stats;
    stats.total_pushed = total_pushed_;
    stats.total_popped = total_popped_;
    stats.current_size = queue_.size();
    stats.min_depth = min_depth_;
    stats.max_depth = max_depth_;
    return stats;
}

} // namespace docscraper::queue
