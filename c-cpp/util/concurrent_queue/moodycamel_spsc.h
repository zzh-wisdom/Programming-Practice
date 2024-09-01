#pragma once

#include "con_queue.h"
#include "MOODYCAMEL_SPSC/readerwriterqueue.h"

class MoodycamelSpsc : public ConQueue
{
private:
    moodycamel::ReaderWriterQueue<const void*, 8192> queue_;
    /* data */
public:
    MoodycamelSpsc(size_t size): queue_(size) {
    }
    ~MoodycamelSpsc(){};

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
        return queue_.max_capacity();
    }
};
