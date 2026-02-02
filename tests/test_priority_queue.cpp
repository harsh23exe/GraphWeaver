// test_priority_queue.cpp - Unit tests for priority queue
#include <gtest/gtest.h>
#include "queue/priority_queue.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace docscraper::queue;
using namespace docscraper::models;

// ============================================================================
// Basic Priority Queue Tests
// ============================================================================

TEST(PriorityQueueTest, EmptyQueue) {
    ThreadSafePriorityQueue pq;
    EXPECT_TRUE(pq.empty());
    EXPECT_EQ(pq.size(), 0);
}

TEST(PriorityQueueTest, PushAndPop) {
    ThreadSafePriorityQueue pq;
    
    pq.push(WorkItem("url1", 1));
    EXPECT_FALSE(pq.empty());
    EXPECT_EQ(pq.size(), 1);
    
    auto item = pq.try_pop_nonblocking();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->url, "url1");
    EXPECT_EQ(item->depth, 1);
    EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueTest, PriorityOrdering) {
    ThreadSafePriorityQueue pq;
    
    // Add items with different depths (lower depth = higher priority)
    pq.push(WorkItem("depth3", 3));
    pq.push(WorkItem("depth1", 1));
    pq.push(WorkItem("depth5", 5));
    pq.push(WorkItem("depth2", 2));
    
    // Should pop in order: depth 1, 2, 3, 5
    auto item1 = pq.try_pop_nonblocking();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1->depth, 1);
    
    auto item2 = pq.try_pop_nonblocking();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2->depth, 2);
    
    auto item3 = pq.try_pop_nonblocking();
    ASSERT_TRUE(item3.has_value());
    EXPECT_EQ(item3->depth, 3);
    
    auto item4 = pq.try_pop_nonblocking();
    ASSERT_TRUE(item4.has_value());
    EXPECT_EQ(item4->depth, 5);
}

TEST(PriorityQueueTest, CustomPriority) {
    ThreadSafePriorityQueue pq;
    
    // Use custom priority (independent of depth)
    pq.push(WorkItem("high_priority", 10, 1));    // depth=10, priority=1
    pq.push(WorkItem("low_priority", 1, 100));    // depth=1, priority=100
    pq.push(WorkItem("medium_priority", 5, 50));  // depth=5, priority=50
    
    // Should pop in priority order: 1, 50, 100
    auto item1 = pq.try_pop_nonblocking();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1->url, "high_priority");
    EXPECT_EQ(item1->priority, 1);
    
    auto item2 = pq.try_pop_nonblocking();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2->url, "medium_priority");
    
    auto item3 = pq.try_pop_nonblocking();
    ASSERT_TRUE(item3.has_value());
    EXPECT_EQ(item3->url, "low_priority");
}

TEST(PriorityQueueTest, PushBatch) {
    ThreadSafePriorityQueue pq;
    
    std::vector<WorkItem> items = {
        WorkItem("url1", 1),
        WorkItem("url2", 2),
        WorkItem("url3", 3)
    };
    
    pq.push_batch(items);
    EXPECT_EQ(pq.size(), 3);
}

TEST(PriorityQueueTest, CloseQueue) {
    ThreadSafePriorityQueue pq;
    pq.push(WorkItem("url", 1));
    
    EXPECT_FALSE(pq.is_closed());
    pq.close();
    EXPECT_TRUE(pq.is_closed());
    
    // Can still pop existing items after close
    auto item = pq.try_pop_nonblocking();
    EXPECT_TRUE(item.has_value());
}

TEST(PriorityQueueTest, Stats) {
    ThreadSafePriorityQueue pq;
    
    pq.push(WorkItem("url1", 1));
    pq.push(WorkItem("url2", 5));
    pq.push(WorkItem("url3", 3));
    
    auto stats = pq.get_stats();
    EXPECT_EQ(stats.total_pushed, 3);
    EXPECT_EQ(stats.total_popped, 0);
    EXPECT_EQ(stats.current_size, 3);
    EXPECT_EQ(stats.min_depth, 1);
    EXPECT_EQ(stats.max_depth, 5);
    
    pq.try_pop_nonblocking();
    stats = pq.get_stats();
    EXPECT_EQ(stats.total_popped, 1);
    EXPECT_EQ(stats.current_size, 2);
}

TEST(PriorityQueueTest, Clear) {
    ThreadSafePriorityQueue pq;
    
    pq.push(WorkItem("url1", 1));
    pq.push(WorkItem("url2", 2));
    EXPECT_EQ(pq.size(), 2);
    
    pq.clear();
    EXPECT_TRUE(pq.empty());
    EXPECT_EQ(pq.size(), 0);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(PriorityQueueTest, ConcurrentPush) {
    ThreadSafePriorityQueue pq;
    const int num_threads = 4;
    const int items_per_thread = 1000;
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&pq, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                pq.push(WorkItem("url_" + std::to_string(t) + "_" + std::to_string(i), i));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(pq.size(), num_threads * items_per_thread);
}

TEST(PriorityQueueTest, ConcurrentPushPop) {
    ThreadSafePriorityQueue pq;
    const int num_items = 1000;
    std::atomic<int> popped_count{0};
    
    // Producer thread
    std::thread producer([&pq, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            pq.push(WorkItem("url_" + std::to_string(i), i));
        }
        // Small delay then close
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        pq.close();
    });
    
    // Consumer threads
    std::vector<std::thread> consumers;
    for (int c = 0; c < 4; ++c) {
        consumers.emplace_back([&pq, &popped_count]() {
            while (true) {
                auto item = pq.try_pop(std::chrono::milliseconds(100));
                if (!item.has_value()) {
                    if (pq.is_closed() && pq.empty()) {
                        break;
                    }
                } else {
                    ++popped_count;
                }
            }
        });
    }
    
    producer.join();
    for (auto& c : consumers) {
        c.join();
    }
    
    EXPECT_EQ(popped_count.load(), num_items);
}

TEST(PriorityQueueTest, BlockingPop) {
    ThreadSafePriorityQueue pq;
    std::atomic<bool> popped{false};
    
    std::thread consumer([&pq, &popped]() {
        auto item = pq.pop();  // Should block until item is pushed
        popped = item.has_value();
    });
    
    // Give consumer time to start blocking
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(popped.load());
    
    // Push an item to unblock
    pq.push(WorkItem("url", 1));
    
    consumer.join();
    EXPECT_TRUE(popped.load());
}

TEST(PriorityQueueTest, BlockingPopClose) {
    ThreadSafePriorityQueue pq;
    std::atomic<bool> returned_nullopt{false};
    
    std::thread consumer([&pq, &returned_nullopt]() {
        auto item = pq.pop();  // Should block until closed
        returned_nullopt = !item.has_value();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pq.close();
    
    consumer.join();
    EXPECT_TRUE(returned_nullopt.load());
}

TEST(PriorityQueueTest, TryPopTimeout) {
    ThreadSafePriorityQueue pq;
    
    auto start = std::chrono::steady_clock::now();
    auto item = pq.try_pop(std::chrono::milliseconds(100));
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_FALSE(item.has_value());
    EXPECT_GE(elapsed, std::chrono::milliseconds(90));
    EXPECT_LE(elapsed, std::chrono::milliseconds(200));
}

// ============================================================================
// Semaphore Tests
// ============================================================================

TEST(SemaphoreTest, BasicAcquireRelease) {
    Semaphore sem(2);
    EXPECT_EQ(sem.available(), 2);
    
    sem.acquire();
    EXPECT_EQ(sem.available(), 1);
    
    sem.acquire();
    EXPECT_EQ(sem.available(), 0);
    
    sem.release();
    EXPECT_EQ(sem.available(), 1);
}

TEST(SemaphoreTest, TryAcquire) {
    Semaphore sem(1);
    
    EXPECT_TRUE(sem.try_acquire());
    EXPECT_FALSE(sem.try_acquire());  // Should fail, no permits available
    
    sem.release();
    EXPECT_TRUE(sem.try_acquire());
}

TEST(SemaphoreTest, TryAcquireTimeout) {
    Semaphore sem(0);  // No permits
    
    auto start = std::chrono::steady_clock::now();
    bool acquired = sem.try_acquire_for(std::chrono::milliseconds(100));
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_FALSE(acquired);
    EXPECT_GE(elapsed, std::chrono::milliseconds(90));
}

TEST(SemaphoreTest, ConcurrentAccess) {
    Semaphore sem(3);
    std::atomic<int> active{0};
    std::atomic<int> max_active{0};
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&sem, &active, &max_active]() {
            sem.acquire();
            
            int current = ++active;
            int expected = max_active.load();
            while (current > expected && 
                   !max_active.compare_exchange_weak(expected, current)) {}
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            --active;
            sem.release();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // At most 3 threads should have been active at once
    EXPECT_LE(max_active.load(), 3);
}

TEST(SemaphoreTest, Guard) {
    Semaphore sem(1);
    
    {
        SemaphoreGuard guard(sem);
        EXPECT_EQ(sem.available(), 0);
    }
    // Guard destroyed, should have released
    EXPECT_EQ(sem.available(), 1);
}

// ============================================================================
// Blocking Queue Tests
// ============================================================================

TEST(BlockingQueueTest, BasicPushPop) {
    BlockingQueue<int> queue;
    
    queue.push(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);
    
    auto item = queue.try_pop();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(*item, 42);
}

TEST(BlockingQueueTest, FIFO) {
    BlockingQueue<int> queue;
    
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    EXPECT_EQ(*queue.try_pop(), 1);
    EXPECT_EQ(*queue.try_pop(), 2);
    EXPECT_EQ(*queue.try_pop(), 3);
}

TEST(BlockingQueueTest, Close) {
    BlockingQueue<int> queue;
    queue.push(1);
    
    queue.close();
    EXPECT_TRUE(queue.is_closed());
    
    // Can still pop existing items
    auto item = queue.try_pop();
    EXPECT_TRUE(item.has_value());
}
