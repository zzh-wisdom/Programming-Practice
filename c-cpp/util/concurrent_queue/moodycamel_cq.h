#pragma once

#include "con_queue.h"
#include "MOODYCAMEL_ConcurrentQueue/concurrentqueue.h"

class MoodycamelCQ : public ConQueue
{
private:
    moodycamel::ConcurrentQueue<const void*> queue_;
    /* data */
public:
    MoodycamelCQ(size_t size): queue_(size) {
    }
    ~MoodycamelCQ(){};

    bool enqueue(const void *val) override {
        return queue_.try_enqueue(val);
    }

	bool dequeue(const void **val) override {
        return queue_.try_dequeue(*val);
    }

	int size() override {
        return queue_.size_approx();
    }

	int capacity() override {
        return -1;
    }
};
