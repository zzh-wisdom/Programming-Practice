#include "linked_queue.h"

#include <assert.h>

#include "bitops/bitops.h"
#include "memory_order/fence.h"

struct LinkedQueue::Node {
    const void* value;
    std::atomic<Node*> next;
    Node(LinkedQueue* q) : value(nullptr), next(nullptr), ref(0), queue(q) {}

    void IncRef(std::memory_order mo = std::memory_order_relaxed) { ref.fetch_add(1, mo); }

    void DecRef() {
        if (ref.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            queue->FreeNode(this);
        }
    }

  private:
    friend class LinkedQueue;
    std::atomic<uint32_t> ref;
    LinkedQueue* queue;
};

void LinkedQueue::InitPool(uint32_t size) {
    return;
    // node_arr_.resize(size);
    // mask_ = size - 1;
    // arr_head_.store(0, std::memory_order_relaxed);
    // arr_tail_.store(0, std::memory_order_relaxed);
    // for(uint32_t i = 0; i < size; ++i) {
    //     node_arr_[i] = new Node(this);
    // }
}

LinkedQueue::Node* LinkedQueue::AllocNode() {
    // // 线程数不会太多，uint32足以保证不会短时间内发生回环
    // uint32_t cur_tail = arr_tail_.fetch_add(1, std::memory_order_relaxed);
    // uint32_t cur_head = arr_head_.load(std::memory_order_acquire);
    // if (cur_tail - cur_head >= node_arr_.size()) {
    //     // 已经满了，回退指针
    //     arr_tail_.fetch_sub(1, std::memory_order_relaxed);
    //     return nullptr;
    // }
    // uint32_t off = cur_tail & mask_;
    // Node* node = node_arr_[off];

    Node* node = new Node(this);

    node->IncRef();
    return node;
}

// FIXME: 释放需要按顺序，可能比较慢。
// 感觉行不通，如果要打破这种顺序限制，又要额外实现一个无锁队列。无锁队列中又有无锁队列，性能会减半吧。。
void LinkedQueue::FreeNode(Node* node) {
    // // assert(node->ref.load(std::memory_order_relaxed) == 0);
    // uint32_t ref = node->ref.load(std::memory_order_relaxed);
    // if(ref != 0) {
    //     printf("ref[%u] != 0\n", ref);
    // }
    // uint32_t cur_head = arr_head_.load(std::memory_order_acquire);
    // uint32_t cur_tail = arr_tail_.load(std::memory_order_acquire);
    // uint32_t new_head = cur_head;
    // while (new_head != cur_tail) {
    //     uint32_t off = new_head & mask_;
    //     if (node_arr_[off]->ref.load(std::memory_order_acquire) != 0) break;
    //     new_head++;
    // }
    // arr_head_.compare_exchange_weak(cur_head, new_head);

    delete node;
}

LinkedQueue::LinkedQueue(uint32_t size) {
    size = next_power_of_2(size);
    InitPool(size);
    Node* node = AllocNode();
    assert(node);
    node->next = nullptr;
    head_.store(node, std::memory_order_release);
    tail_.store(node, std::memory_order_release);
}

LinkedQueue::~LinkedQueue() {
    // for (uint32_t i = 0; i < node_arr_.size(); ++i) {
    //     delete node_arr_[i];
    // }
}

LinkedQueue::Node* LinkedQueue::SafeRead(std::atomic<Node*> &q) {
    while(true) {
        Node* p = q.load(std::memory_order_acquire);
        if (p == NULL){
            return p;
        }
        p->IncRef(std::memory_order_seq_cst);
        // 这里涉及storeload栅栏，可能有bug
        if (p == q.load(std::memory_order_seq_cst)){
            return p;
        }else{
            p->DecRef();
        }
    }
}

class LinkedQueue::NodeRefGuard {
public:
    NodeRefGuard(LinkedQueue::Node* node) : node_(node) {}
    ~NodeRefGuard() { node_->DecRef(); }
private:
    LinkedQueue::Node* node_;
};


bool LinkedQueue::enqueue(const void* val) {
    Node* n = AllocNode();
    if (!n) return false;
    n->value = val;
    n->next = nullptr;
    acquire_fence();
    Node* tail = nullptr;
    while (true) {
        // 先取一下尾指针和尾指针的next
        tail = tail_.load(std::memory_order_acquire);
        // tail = SafeRead(tail_);
        // NodeRefGuard g(tail);
        Node* next = tail->next.load(std::memory_order_acquire);

        // 如果尾指针已经被移动了，则重新开始
        if (tail != tail_.load(std::memory_order_acquire)) continue;

        // 如果尾指针的 next 不为NULL，则 fetch 全局尾指针到next
        if (next != NULL) {
            tail_.compare_exchange_weak(tail, next, std::memory_order_release, std::memory_order_relaxed);
            continue;
        }
        // 如果加入结点成功，则退出
        if (tail->next.compare_exchange_weak(next, n, std::memory_order_release, std::memory_order_relaxed) == true) break;
    }
    tail_.compare_exchange_weak(tail, n, std::memory_order_release, std::memory_order_relaxed);
    return true;
}

bool LinkedQueue::dequeue(const void** val) {
    Node* head = nullptr;
    while (true) {
        // 取出头指针，尾指针，和第一个元素的指针
        head = head_.load(std::memory_order_acquire);
        Node* tail = tail_.load(std::memory_order_acquire);
        // head = SafeRead(head_); 
        // Node* tail = SafeRead(tail_);
        // NodeRefGuard g1(head);
        // NodeRefGuard g2(tail);
        Node* next = head->next.load(std::memory_order_acquire);

        // Q->head 指针已移动，重新取 head指针
        if (head != head_.load(std::memory_order_acquire)) continue;

        // 如果是空队列
        if (head == tail && next == nullptr) {
            return false;
        }

        // 如果 tail 指针落后了
        if (head == tail && next != nullptr) {
            tail_.compare_exchange_weak(tail, next, std::memory_order_release, std::memory_order_relaxed);
            continue;
        }
        // 移动 head 指针成功后，取出数据
        if (head_.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_relaxed) == true) {
            *val = next->value;
            break;
        }
    }
    // head->DecRef();  // 释放老的dummy结点
    FreeNode(head);
    return true;
}

int LinkedQueue::size() { return -1; }

int LinkedQueue::capacity() { 
    return -1;
    // return node_arr_.size() - 1; 
}
