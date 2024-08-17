#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace std;

class Semaphore {
  public:
    Semaphore(long count = 0) : count(count) {}
    // V操作，唤醒
    void signal() {
        unique_lock<mutex> unique(mt);
        ++count;
        if (count <= 0) cond.notify_one();
    }
    // P操作，阻塞
    void wait() {
        unique_lock<mutex> unique(mt);
        --count;
        // cond.wait(unique, [&]{ return count >= 0; });
        if (count < 0) cond.wait(unique);
    }

  private:
    mutex mt;
    condition_variable cond;
    long count;
};

Semaphore beginSema1;
Semaphore beginSema2;
Semaphore endSema;

int X, Y;
int r1, r2;

void* thread1Func(void* param) {
    while (1) {
        beginSema1.wait();
        while ((rand() / (double)RAND_MAX) > 0.2);
        X = 1;
        __asm__ __volatile__("" ::: "memory");
        std::atomic_thread_fence(std::memory_order_seq_cst);
        // __asm__ volatile("mfence":::"memory");
        r1 = Y;
        endSema.signal();
    }
    return NULL;
}

void* thread2Func(void* param) {
    while (1) {
        beginSema2.wait();
        while ((rand() / (double)RAND_MAX) > 0.2);
        Y = 1;
        __asm__ __volatile__("" ::: "memory");
        std::atomic_thread_fence(std::memory_order_seq_cst);
        r2 = X;
        endSema.signal();
    }
    return NULL;
}

int main() {

    thread thread1(thread1Func, nullptr);
    thread thread2(thread2Func, nullptr);

    int detected = 0;
    int iterations = 0;
    for (iterations = 1;; iterations++) {
        X = 0;
        Y = 0;
        beginSema1.signal();
        beginSema2.signal();
        endSema.wait();
        endSema.wait();
        // std::atomic_thread_fence(std::memory_order_seq_cst);
        if (r1 == 0 && r2 == 0) {
            detected++;
            printf("%d reorders detected after %d iterations\n", detected,
                   iterations);
        } else {
            // printf("%d reorders r1 = %d r2 = %d after %d iterations\n", detected, r1, r2,
            //        iterations);
        }
    }
    return 0;
}