#pragma once

#include <atomic>
#include <vector>

#include "con_queue.h"

// FIXME: 存在并发时原子操作的ABA问题，存在释放读的问题，这种链表的设计模式存在诸多问题
// 用于单生产者单消费者还是没问题的，但是性能又远不如kfifo
// 综合来看基于链表的无锁并发队列 实现困难，且 性能也不好
class LinkedQueue : public ConQueue
{
private:
    struct Node;
    class NodeRefGuard;
    // // 实现node池化的相关成员
    // std::vector<Node*> node_arr_;
    // uint32_t mask_;
    // std::atomic<uint32_t> arr_head_;
    // std::atomic<uint32_t> arr_tail_;


    // 实现无锁链表的相关成员
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

    // // size 需要为2的幂次
    void InitPool(uint32_t size);
    Node* AllocNode();
    void FreeNode(Node* node);

    Node* SafeRead(std::atomic<Node*> &q);
public:
    LinkedQueue(uint32_t size);

    LinkedQueue(const LinkedQueue&) = delete;
    LinkedQueue& operator=(const LinkedQueue&) = delete;

    ~LinkedQueue();

    bool enqueue(const void *val) override;

	bool dequeue(const void **val) override;

	int size() override;

	int capacity() override;
};
