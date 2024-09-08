#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "con_queue.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define cpu_pause() asm volatile("pause" : : : "memory")
const int CPM_NUM = 32;

class RteRing : public ConQueue {
  private:
    std::vector<const void*> base_;
    uint32_t mask_;
    std::atomic<uint32_t> prod_head_;
    std::atomic<uint32_t> prod_tail_;
    std::atomic<uint32_t> cons_head_;
    std::atomic<uint32_t> cons_tail_;

  public:
    RteRing(size_t size)
        : base_(size),
          mask_(size - 1),
          prod_head_(0),
          prod_tail_(0),
          cons_head_(0),
          cons_tail_(0) {}
    ~RteRing() {};

    bool enqueue(const void* val) override {
        bool success = false;
        uint32_t prod_head;
        uint32_t prod_next;
        // return (fifo->mask + 1) - (fifo->in - fifo->out);
        do {
            prod_head = prod_head_.load(std::memory_order_acquire);
            uint32_t cons_tail = cons_tail_.load(std::memory_order_acquire);
            uint32_t free_entries = base_.size() - (prod_head - cons_tail);
            if (free_entries == 0) {
                return false;
            }
            prod_next = prod_head + 1;
            success = prod_head_.compare_exchange_strong(
                prod_head, prod_next, std::memory_order_release,
                std::memory_order_acquire);
        } while (unlikely(success == 0));

        base_[prod_head & mask_] = val;

        int cmp_num = 0;
        while (
            unlikely(prod_tail_.load(std::memory_order_acquire) != prod_head)) {
            // cpu_pause();
            cmp_num++;
            if (cmp_num > CPM_NUM) {
                std::this_thread::yield();
            }
        }

        prod_tail_.store(prod_next, std::memory_order_release);
        return true;
    }

    bool dequeue(const void** val) override {
        bool success = false;
        uint32_t cons_head;
        uint32_t cons_next;
        do {
            cons_head = cons_head_.load(std::memory_order_acquire);
            uint32_t prod_tail = prod_tail_.load(std::memory_order_acquire);
            uint32_t entries = prod_tail - cons_head;
            if (entries == 0) {
                return false;
            }
            cons_next = cons_head + 1;
            success = cons_head_.compare_exchange_strong(
                cons_head, cons_next, std::memory_order_release,
                std::memory_order_acquire);
        } while (unlikely(success == 0));

        *val = base_[cons_head & mask_];

        int cmp_num = 0;
        while (
            unlikely(cons_tail_.load(std::memory_order_acquire) != cons_head)) {
            // cpu_pause();
            cmp_num++;
            if (cmp_num > CPM_NUM) {
                std::this_thread::yield();
            }
        }

        cons_tail_.store(cons_next, std::memory_order_release);
        return true;
    }

    int size() override {
        return prod_tail_.load(std::memory_order_acquire) -
               cons_tail_.load(std::memory_order_acquire);
    }

    int capacity() override { return base_.size(); }
};
