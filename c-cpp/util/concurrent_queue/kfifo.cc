// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A generic kernel FIFO implementation
 *
 * Copyright (C) 2009/2010 Stefani Seibold <stefani@seibold.net>
 */

#include "kfifo.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>

#include "bitops/bitops.h"
#include "memory_order/fence.h"

/*
 * internal helper to calculate the unused elements in a fifo
 */
static inline unsigned int kfifo_unused(struct __kfifo* fifo) {
    return (fifo->mask + 1) - (fifo->in - fifo->out);
}

// size不能小于2
int __kfifo_alloc(struct __kfifo* fifo, unsigned int size) {
    /*
     * round up to the next power of 2, since our 'let the indices
     * wrap' technique works only in this case.
     */
    size = next_power_of_2(size);

    fifo->in = 0;
    fifo->out = 0;

    if (size < 2) {
        fifo->data = nullptr;
        fifo->mask = 0;
        return EINVAL;
    }

    fifo->data = (const void**)malloc(sizeof(void*) * size);

    if (!fifo->data) {
        fifo->mask = 0;
        return ENOMEM;
    }
    fifo->mask = size - 1;

    return 0;
}

void __kfifo_free(struct __kfifo* fifo) {
    if (fifo->data) {
        free(fifo->data);
        fifo->data = nullptr;
    }
    fifo->in = 0;
    fifo->out = 0;
    fifo->mask = 0;
}

bool __kfifo_in(struct __kfifo* fifo, const void* val) {
    if (kfifo_unused(fifo) == 0) return false;

    uint32_t off = fifo->in & fifo->mask;

    // acquire_fence();
    // 猜想：这里if else 可以防止store指令重排

    fifo->data[off] = val;

    /*
     * make sure that the data in the fifo is up to date before
     * incrementing the fifo->in index counter
     */
    release_fence();

    fifo->in += 1;
    return true;
}

bool __kfifo_out_peek(struct __kfifo* fifo, const void** val) {
    if (kfifo_is_empty(*fifo)) return false;

    uint32_t off = fifo->out & fifo->mask;

    acquire_fence(); // 预防 fifo->data[off]的读取 被重排到  fifo->in的读取 之前

    *val = fifo->data[off];

    return true;
}

bool __kfifo_out(struct __kfifo* fifo, const void** val) {
    if (!__kfifo_out_peek(fifo, val)) return false;

    /*
     * make sure that the data is copied before
     * incrementing the fifo->out index counter
     */
    release_fence();

    fifo->out += 1;

    return true;
}

KFifo::KFifo(uint32_t size) { __kfifo_alloc(&fifo_, size); }

KFifo::~KFifo() { __kfifo_free(&fifo_); }

bool KFifo::enqueue(const void* val) {
	return __kfifo_in(&fifo_, val);
}

bool KFifo::dequeue(const void** val) {
	return __kfifo_out(&fifo_, val);
}
