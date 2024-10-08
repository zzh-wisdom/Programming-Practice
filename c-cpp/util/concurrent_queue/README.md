# 并发队列

参考

1. [并发无锁队列学习之一【开篇】](https://www.cnblogs.com/alantu2018/p/8469168.html)

线程间通信，网络通信。缓解数据处理压力。

1. 4种场景：单生产者——单消费者、多生产者——单消费者、单生产者——多消费者、多生产者——多消费者。可归纳为：
   1. 单对单
   2. 多对多

2. 根据队列中数据分为：队列中的数据是**定长的**、队列中的数据是**变长的**。

## 单对单——无锁并发队列

linux内核提供的kfifo的实现
[巧夺天工的kfifo(修订版）](https://blog.csdn.net/linyt/article/details/53355355)

功能：无锁字节流服务

1. fifo->in和out指针均为无符号整数，不进行回环处理，单调递增，即使发生无符号数回绕也不会有问题。因此不需要预留1个空间用来区分满/空

https://github.com/torvalds/linux/blob/v6.10/lib/kfifo.c#L394
1. 新版代码添加了recsize（最多2字节），用于识别字节流中的record。
2. 去除了许多内存屏障

[内存屏障(Memory Barriers)](https://www.cnblogs.com/icanth/archive/2012/06/10/2544300.html)
简单理解就是：

mb - mfence
rmb - lfence
wmb - sfence

## 多对多

正常逻辑操作是要对队列操作进行加锁处理。加锁的性能开销较大，一般采用无锁实现。无锁实现原理是CAS、FAA等机制。

使用链表实现多对多的变长无锁队列，参考：https://coolshell.cn/articles/8239.html
额外参考（性能并不最佳）：https://moodycamel.com/blog/2014/solving-the-aba-problem-for-lock-free-free-lists

使用链表的方式实现变长无锁队列有一些缺点：

1. 频繁申请和释放空间，若配合内存池则有空间浪费问题
2. ABA问题，需要解决
3. 变长，不适合限制资源增长

推荐使用数组方式实现定长无锁并发队列，具体有两种方法：

1. 用数组作为资源池实现定长的链表式无锁队列。(废弃)
   1. 自己基于《[无锁队列的实现》](https://coolshell.cn/articles/8239.html)中无锁链表的实现，进行改造。
   2. 但仍然有ABA问题，需要用引用计数防止释放的方式实现
   3. 存在问题：池化实现比较复杂，开销太大
2. 论文《Implementing Lock-Free Queues》的实现，需要使用double-CAS，先不考虑
3. intel dpdk提供的rte_ring实现：http://blog.csdn.net/linzhaolover/article/details/9771329

目前结论：不推荐使用链表实现无锁队列，问题太多

## 工业级实现

moodycamel::ConcurrentQueue 实现

1. 单生产者单消费者：https://github.com/cameron314/readerwriterqueue
2. 多生产者多消费者：https://github.com/cameron314/concurrentqueue?tab=readme-ov-file

相关博客：
https://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++
https://moodycamel.com/blog/2014/detailed-design-of-a-lock-free-queue

### 主要技术

1. 通过数组来实现（非链表实现）
2. cacheline对齐，避免频繁的缓存行失效(该技术实践中发现有可能会反向优化，适用于高竞争场景)
3. 通过**循环链表**连接的block来实现空间自动扩展
4. weak_atomic 和 LightweightSemaphore（不会立即睡眠，而是先忙等一阵子） 轮子
5. 影子tail和front指针，减少原子变量的竞争访问
6. 通过条件变量，实现阻塞版本：BlockingReaderWriterQueue

其他实现技巧：

1. C++中柔性数组如何进行内存分配和对齐

TODO: 详细阅读代码

### 存在缺点

1. 旧的块不会释放，可能会导致内存使用越来越多。（即capacity只会变大而不会缩小）
2. 多生产者多消费者版本，不保证线性一致性。（但会按照单个生产者的顺序出队）

### 多生产者多消费者设计

元素在内部使用连续块而不是链表存储，以获得更好的性能。
该队列由**一组子队列**组成，每个子队列对应一个生产者。当消费者想要将一个元素出队时，它会检查所有子队列，直到找到一个不为空的子队列。然而，所有这些对于队列的用户来说基本上都是透明的。

然而，这种设计的一个特殊后果（似乎不直观）是，如果两个生产者同时入队，则当元素稍后出队时，元素之间没有定义的顺序。通常这很好，因为即使使用完全线性化的队列，生产者线程之间也会存在竞争，因此您无论如何都不能依赖顺序。但是，如果由于某种原因您自己在两个生产者线程之间进行额外的显式同步，从而定义入队操作之间的总顺序，您可能会期望元素将以相同的总顺序出现，这个保证我的队列不会提供。

支持按令牌指定生产者的子队列。为了避免子队列数量无限制地增长，隐式生产者被标记为在线程退出后重用。但是，并非所有平台都支持此功能。如果从短期线程使用队列，建议改用显式生产者令牌。

每个生产者都需要一些线程本地数据，线程本地数据也可用于加速消费者。这种线程本地数据可以与用户分配的令牌相关联，或者，为了简化接口，如果用户没有为生产者提供令牌，**则使用无锁哈希表（以当前线程 ID 为键）来查找线程本地生产者队列**： 为每个显式分配的生产者标记创建一个 SPMC 队列，为每个生产项目而不提供标记的线程创建另一个隐式队列（这么说一个线程可能会对应两个SPMC，一个是显示另一个是隐式）。由于令牌包含相当于线程特定的数据，因此绝不能在多个线程中同时使用（不过可以将令牌的所有权转移给另一个线程；特别是，这样可以在**线程池**任务中使用令牌，即使运行任务的线程中途发生了变化）。

显式生产者队列和隐式生产者队列的生命周期有本质区别：显式生产者队列的有限生产生命周期与令牌的生命周期相关联，**而隐式生产者队列的生命周期则不受限制，在高级队列本身存在的时间内都存在**。因此，我们使用了两种略有不同的 SPMC 算法，以最大限度地提高速度和内存使用率。一般来说，显式生产者队列的速度稍快，占用的内存稍多，**而隐式生产者队列的速度稍慢，但能将更多内存回收到高层队列的全局池中**。为了获得最佳速度，请始终使用显式标记（除非你觉得太不方便）。

| 初步感觉：只有隐式生产者才会用到全局的block池。

生产时：如果索引中没有足够的空间来添加新项目，则会分配一个新的索引数组，该数组的大小是前一个数组的两倍（显然，只有在允许内存分配的情况下才允许这样做 - 如果不允许，则整个队列操作正常失败）。

- 显示生产者：显式生产者队列需要在入队时传递用户分配的“生产者令牌”。该令牌仅包含指向生产者队列对象的指针。创建token时，会创建对应的生产者队列；当令牌被销毁时，生产者队列可能仍然包含未使用的元素，因此队列本身比令牌寿命更长。事实上，**一旦分配，生产者队列就永远不会被销毁（直到高级队列被销毁）**，但它会在下次创建生产者令牌时重新使用（而不是导致为新的生产者队列进行堆分配） ）。
- 隐式生产者：隐式生产者队列被实现为**一组未链接的块**。维护一个current block指针；这是当前正在排队的块。当一个块填满时，会申请一个新的块，并且旧的块（从生产者的角度来看）会被遗忘。在将元素添加到块之前，该块会被插入到块索引中（这允许消费者找到生产者已经忘记的块）。当块中的最后一个元素被消耗完毕时，该块将从逻辑上从块索引中删除。
  - 隐式生产者队列永远不会重复使用——一旦创建，它就会在高级队列的整个生命周期中存在。因此，为了减少内存消耗，它不会占用曾经使用过的所有块（如显式生产者），而是将用过的块返回到全局空闲列表。为此，一旦消费者完成将项目出队，每个块中的原子出队计数就会增加；当计数器达到块的大小时，看到这一点的消费者知道它刚刚使最后一个项目出队，并将该块放入全局空闲列表中。
  - 隐式生产者实现为一个大的循环缓冲区，包换block index，实际就是一个数组，元素中存放tail_index + block指针

当消费者想要将一个项目出列时，它只需遍历生产者列表，寻找其中包含项目的 SPMC 队列（由于生产者的数量是无限的，这部分是使高级队列只是无锁的原因）免等待）。

在产生或消耗大量元素时，最有效的方法是：

1. 使用带有标记的队列批量方法
2. 如果不行，使用不带标记的批量方法
3. 否则，使用带标记的单项方法
4. 否则，使用不带标记的单项方法

并非所有生产者都拥有相同数量的可用元素，并且某些消费者可能比其他消费者消耗得更快；为了解决这个问题，**第一个从同一内部生产者队列连续消费 256 个项目的消费者会增加全局偏移量，从而导致所有消费者在下一个出列操作上轮换并开始从下一个生产者消费**。 （请注意，这意味着轮换速率由最快的使用者决定。）如果使用者的指定队列上没有可用元素，它将移至下一个有可用元素的队列。这种简单的启发式方法非常高效，能够以近乎完美的扩展方式将消费者与生产者配对，从而带来令人印象深刻的出队加速。

### 性能启发

我认为，从这些基准中可以得出的最重要的结论是，**随着线程数量的增加，系统总吞吐量最多只能保持基本不变（而且经常会下降）**。这意味着，即使使用每个基准的最快队列，每个线程单独完成的工作量也会随着线程数量的增加而减少，例如四个线程和十六个线程完成的工作量总和大致相同。这还只是最好的情况。在这种争用和吞吐量水平上，不存在线性扩展；这个故事的寓意是，如果你关心性能，就不要采用需要密集共享数据的设计。当然，实际应用往往不是纯粹在队列中移动，因此争用程度要低得多，而且随着线程的增加，确实有可能向上扩展（至少有一点）。

### 自带的FreeList实现，避免ABA问题

https://moodycamel.com/blog/2014/solving-the-aba-problem-for-lock-free-free-lists

### 隐式生产者队列的哈希值(值得参考)

基于 [Jeff Preshing's lock-free hash algorithm](https://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table/),无锁哈希表用于将线程 ID 映射到隐式生产者；当没有为各种排队方法提供显式生产者令牌时，使用此方法。元素永远不会被删除

## 其他相关实现

1. glib async_queue（简单队列+lock和条件变量来实现的）：https://blog.csdn.net/farsight_2098/article/details/135017132
2. Yet another implementation of a lock-free circular array queue: https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circul
3. 

## ABA问题

问题的原理：https://blog.csdn.net/weixin_34309543/article/details/94260402
![](images/Markdown-image-2024-08-18-18-28-42.png)

无锁栈的场景比较好理解：线程1想出栈，准备CAS设置新的head时，线程2接连pop A、pop B，delete B，push A。此时线程1错误以为head没有改过，改新head设置为B，造成操作。

综合来看还是方法1比较好。代价是浪费一些内存。或者还是用数组实现为定长队列吧。

1. 以无锁方式循环节点的一种很自然的办法就是让每个线程维护它自己的由未使用队列项所组成的私有空闲链表。
当一个入队线程需要一个新节点时，它尝试从线程本地空闲链表中删除一个节点。如果空闲链表为空， 就new一个新节点。当一个出队线程准备释放一个节点时，它将该节点放入线程本地空闲链表。因为链表是线程本地的，因此不需要很大的同步开销。只要每个线程入队和出队次数大致相同，这种设计的效果就非常好。如果两种操作次数不平衡，则需要更加复杂的技术。
1. double CAS（每一次赋值，将指针的count+1赋值给被赋值指针）。_InterlockedCompareExchange128，但128位原子操作可能有性能问题
https://zhuanlan.zhihu.com/p/352723264

```cpp
CAS(&tail.ptr–>next, next, <node, next.count+1>)
CAS(&Q–>Head, head, <next.ptr, head.count+1>)
```

其中next和head是将要赋的值，要将其中的count+1后赋值目标指针。（绕口..）

3. 让被访问的指针不被释放。

缺点是每次访问指针都多好多次原子操作。

```cpp
SafeRead(q)
{
    loop:
        p = q->next;
        if (p == NULL){
            return p;
        }

        Fetch&Add(p->refcnt, 1);

        if (p == q->next){
            return p;
        }else{
            Release(p);
        }
    goto loop;
}
```
其中的 Fetch&Add和Release分是是加引用计数和减引用计数，都是原子操作，这样就可以阻止内存被回收了。

## 其他参考

1. Glib Asynchronous Queues: https://docs.gtk.org/glib/
2. glib的介绍： https://blog.csdn.net/yetugeng/article/details/87861993
3. boost 库
4. 