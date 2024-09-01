#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "util/util.h"

#include "concurrent_queue/kfifo.h"
#include "concurrent_queue/arr_base_linked_queue.h"
#include "concurrent_queue/moodycamel_spsc.h"

uint64_t type;
uint64_t size;
uint64_t op_num;
uint64_t run_time = 60;  // 单位为秒
std::vector<std::thread> threads;
std::vector<ThreadInfo> thread_infos;
ConQueue* queue = nullptr;
std::atomic<bool> quit(false);

void Stop(uint64_t delay) {
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    quit.store(true, std::memory_order_relaxed);
    std::cout << "Time Out" << std::endl;
}

void Enqueue(ThreadInfo& thread_info) {
    uint64_t i = 0;
    while (i < op_num) {
        void* val = (void*)(uintptr_t)i;
        if (!queue->enqueue(val)) continue;
        // printf("enqueue %d\n", i);
        i++;
        thread_info.count++;
    }
    return;
}

void Dequeue(ThreadInfo& thread_info) {
    uint64_t i = 0;
    while (i < op_num) {
        const void* val;
        if (!queue->dequeue(&val)) continue;
        uint64_t ret = (uint64_t)(uintptr_t)val;
        if (ret != i) {
            std::cout << "Test Fail!" << std::endl;
            printf("ret[%llu] != i[%llu]\n", ret, i);
            exit(1);
        }
        // printf("dequeue %d\n", i);
        i++;
        thread_info.count++;

        if(i % 10000000 == 0) {
            printf("queue size: %u, count: %llu\n", queue->size(), thread_info.count);
        }
    }
    return;
}

void InitQueue() {
    switch (type) 
    {
    case 1:
        queue = new KFifo(size);
        break;
    case 2:
        queue = new ArrayBaseLinkedQueue(size);
        break;
    case 3:
        queue = new MoodycamelSpsc(size);
        break;
    default:
        printf("Not support type: %llu\n", type);
        exit(1);
        break;
    }
}

// ./bazel-bin/util/test/spsc_test 1 1024 1000000000
// macOS笔记本上的最优size: 4096
// ./bazel-bin/util/test/spsc_test 1 4096 1000000000
// avg: 7.19783 nsec/op
// ./bazel-bin/util/test/spsc_test 2 4096 100000000
// avg: 75.3207 nsec/op
// ./bazel-bin/util/test/spsc_test 3 4096 100000000
// avg: 8.64263 nsec/op
int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s <type: 1-kfifo, 2-arr_base_linked_queue, 3-moodycamel_spsc> <size> <op_num>\n",
               argv[0]);
        return -1;
    }
    type = atoi(argv[1]);
    size = atoi(argv[2]);
    op_num = atoi(argv[3]);
    InitQueue();
    printf("type: %llu, queue capacity: %d, op_num: %llu\n", type, queue->capacity(), op_num);

    thread_infos.resize(2);

    auto start_times = NowNanos();
    threads.push_back(std::thread(Enqueue, std::ref(thread_infos[0])));
    threads.push_back(std::thread(Dequeue, std::ref(thread_infos[1])));
    for (auto& thread : threads) {
        thread.join();
    }
    auto end_times = NowNanos();

    double cost_sec = (end_times - start_times) * 1.0 / 1000000000;

    std::cout << "cost time: " << cost_sec << " s" << std::endl;
    std::cout << "Average Throughput: "
              << (uint64_t)(op_num * 2 / cost_sec)
              << " ops/sec " << std::endl;

    std::cout << "Average Latency: " << (end_times - start_times) * 1.0 / (op_num * 2) << " ns"
              << std::endl;

    std::cout << "Test Success!" << std::endl;
    return 0;
}