#pragma once

class ConQueue {
  public:
    ConQueue() = default;

    ConQueue(const ConQueue&) = delete;
    ConQueue& operator=(const ConQueue&) = delete;

    virtual ~ConQueue() = default;

    virtual bool enqueue(const void* val) = 0;

    virtual bool dequeue(const void** val) = 0;
};
