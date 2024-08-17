#pragma once

#include <atomic>

#define acquire_fence() std::atomic_thread_fence(std::memory_order_acquire)
#define release_fence() std::atomic_thread_fence(std::memory_order_release)
#define full_fence() std::atomic_thread_fence(std::memory_order_seq_cst)
