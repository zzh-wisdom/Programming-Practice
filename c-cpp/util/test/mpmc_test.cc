#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "util/util.h"

#include "concurrent_queue/kfifo.h"
#include "concurrent_queue/arr_base_linked_queue.h"

uint64_t type;
uint64_t size;
uint64_t op_num;
int producer_num;
int consumer_num;
uint64_t run_time = 60;  // 单位为秒
std::vector<std::thread> threads;
std::vector<ThreadInfo> producer_thread_info;
std::vector<ThreadInfo> consumer_thread_info;
ConQueue* queue = nullptr;
std::atomic<bool> quit(false);

void Stop(uint64_t delay) {
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    quit.store(true, std::memory_order_relaxed);
    std::cout << "Time Out" << std::endl;
}

void Enqueue(ThreadInfo& thread_info) {
    printf("Enqueue start, thread_id: %d\n", thread_info.thread_id);
    uint64_t i = 0;
    while (i < op_num) {
        void* val = (void*)(uintptr_t)i;
        if (!queue->enqueue(val)) continue;
        if(i%100 == 0) printf("enqueue %llu\n", i);
        i++;
        thread_info.count++;
    }
    printf("Enqueue over, thread_id: %d\n", thread_info.thread_id);
    return;
}

void Dequeue(ThreadInfo& thread_info) {
    printf("Dequeue start, thread_id: %d\n", thread_info.thread_id);
    uint64_t i = 0;
    while (i < op_num) {
        const void* val;
        if (!queue->dequeue(&val)) continue;
        uint64_t ret = (uint64_t)(uintptr_t)val;
        if (ret != i) {
            // TODO: 检查ret
            // std::cout << "Test Fail!" << std::endl;
            // printf("ret[%llu] != i[%llu]\n", ret, i);
            // exit(1);
        }
        if(i%100 == 0) printf("dequeue %llu\n", i);
        i++;
        thread_info.count++;

        if(i % 1000000 == 0) {
            printf("queue size: %u, count: %llu\n", queue->size(), thread_info.count);
        }
    }
    printf("Dequeue over, thread_id: %d\n", thread_info.thread_id);
    return;
}

void InitQueue() {
    switch (type) 
    {
    case 1:
        queue = new ArrayBaseLinkedQueue(size);
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
int main(int argc, char** argv) {
    if (argc < 6) {
        printf("Usage: %s <type: 1-std::arr_base_linked_queue> <size> <op_num> <producer_num> <consumer_num>\n",
               argv[0]);
        return -1;
    }
    type = atoi(argv[1]);
    size = atoi(argv[2]);
    op_num = atoi(argv[3]);
    producer_num = atoi(argv[4]);
    consumer_num = atoi(argv[5]);
    InitQueue();
    printf("type: %llu, queue capacity: %d, op_num: %llu\n", type, queue->capacity(), op_num);

    producer_thread_info.resize(producer_num);
    consumer_thread_info.resize(consumer_num);
    for(int i = 0; i < producer_num; i++) {
        producer_thread_info[i].thread_id = i;
    }
    for(int i = 0; i < consumer_num; i++) {
        consumer_thread_info[i].thread_id = i;
    }

    auto start_times = NowNanos();
    for(int i = 0; i < producer_num; i++) {
        threads.push_back(std::thread(Enqueue, std::ref(producer_thread_info[i])));
    }

    for(int i = 0; i < consumer_num; i++) {
        threads.push_back(std::thread(Dequeue, std::ref(consumer_thread_info[i])));
    }
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

    std::cout << "Write Test Success!" << std::endl;
    return 0;
}