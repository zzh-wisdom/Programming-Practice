#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <cstdio>

#include "util/util.h"

#include "concurrent_queue/kfifo.h"
#include "concurrent_queue/arr_base_linked_queue.h"
#include "concurrent_queue/linked_queue.h"

const int MAX_THREAD_NUM = 128;
uint64_t type;
uint64_t size;
uint64_t op_num_per_thread;
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
    uint64_t thread_id = thread_info.thread_id;
    uint64_t last_val = thread_id;
    while (i < op_num_per_thread) {
        uint64_t cur_val = (i << 8) | thread_id;
        if (cur_val != thread_id && cur_val <= last_val) {
            std::cout << "Test Fail!" << std::endl;
            printf("cur_val[%llu] <= last_val[%llu]\n", cur_val, last_val);
            exit(1);
        }
        void* val = (void*)(uintptr_t)cur_val;
        if (!queue->enqueue(val)) continue;
        // if(i%100 == 0) printf("enqueue %llu\n", i);
        i++;
        thread_info.count++;
        last_val = cur_val;
    }
    printf("Enqueue over, thread_id: %d\n", thread_info.thread_id);
    return;
}

void Dequeue(ThreadInfo& thread_info) {
    std::vector<uint64_t> last_seqs;
    last_seqs.resize(MAX_THREAD_NUM);
    printf("Dequeue start, thread_id: %d\n", thread_info.thread_id);
    uint64_t i = 0;
    while (i < op_num_per_thread) {
        const void* val;
        if (!queue->dequeue(&val)) continue;
        uint64_t ret = (uint64_t)(uintptr_t)val;
        uint64_t thread_id = ret & 0xff;
        uint64_t seq = ret >> 8;
        if (seq != 0 &&  seq <= last_seqs[thread_id]) {
            std::cout << "Test Fail!" << std::endl;
            printf("thread_id=%llu, seq[%llu] <= last_seqs[%llu]\n", thread_id, seq, last_seqs[thread_id]);
            exit(1);
        }
        last_seqs[thread_id] = seq;

        // if(i%100 == 0) printf("dequeue %llu\n", i);
        i++;
        thread_info.count++;

        if(i % 10000000 == 0) {
            printf("queue size: %u, count: %llu\n", queue->size(), thread_info.count);
        }
    }
    printf("Dequeue over, thread_id: %d\n", thread_info.thread_id);
    return;
}

void checkParams() {
    if (producer_num > MAX_THREAD_NUM || consumer_num > MAX_THREAD_NUM) {
        printf("Not support producer_num or consumer_num > %d\n", MAX_THREAD_NUM);
        exit(1);
    }
    return;
}

void InitQueue() {
    switch (type) 
    {
    case 0:
        queue = new KFifo(size);
        break;
    case 1:
        queue = new ArrayBaseLinkedQueue(size);
        break;
    case 2:
        queue = new LinkedQueue(size);
        break;
    default:
        printf("Not support type: %llu\n", type);
        exit(1);
        break;
    }
}

// 性能数据：
// MacOS
// kfifo：./bazel-bin/util/test/mpmc_test 0 4096 100000000 1 1
//      146256456 ops/sec 6.8373 ns
// linked_queue: ./bazel-bin/util/test/mpmc_test 2 4096 100000000 1 1
//     1-1: 15296317 ops/sec 65.3752 ns

// ./bazel-bin/util/test/mpmc_test 2 4096 1000000 2 2
int main(int argc, char** argv) {
    if (argc < 6) {
        // kfifo 只支持单生产者单消费者
        printf("Usage: %s <type: 0-kfifo, 1-arr_base_linked_queue, 2-linked_queue> <size> <op_num_per_thread> <producer_num> <consumer_num>\n",
               argv[0]);
        return -1;
    }
    type = atoi(argv[1]);
    size = atoi(argv[2]);
    op_num_per_thread = atoi(argv[3]);
    producer_num = atoi(argv[4]);
    consumer_num = atoi(argv[5]);
    printf("type: %llu, size: %llu, op_num_per_thread: %llu, producer_num: %d, consumer_num: %d\n", type, size, op_num_per_thread, producer_num, consumer_num);
    checkParams();
    InitQueue();
    printf("queue size: %d\n", queue->capacity());

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
    uint64_t op_num = op_num_per_thread * (producer_num + consumer_num);
    std::cout << "total op_num: " << op_num  << std::endl;
    std::cout << "cost time: " << cost_sec << " s" << std::endl;
    std::cout << "Average Throughput: "
              << (uint64_t)(op_num / cost_sec)
              << " ops/sec " << std::endl;

    std::cout << "Average Latency: " << (end_times - start_times) * 1.0 / (op_num) << " ns"
              << std::endl;

    std::cout << "Test Success!" << std::endl;
    return 0;
}