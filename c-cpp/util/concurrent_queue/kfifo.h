/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * A generic kernel FIFO implementation
 *
 * Copyright (C) 2013 Stefani Seibold <stefani@seibold.net>
 */

#pragma once

#include <stdint.h>

/*
 * How to porting drivers to the new generic FIFO API:
 *
 * - Modify the declaration of the "struct kfifo *" object into a
 *   in-place "struct kfifo" object
 * - Init the in-place object with kfifo_alloc() or kfifo_init()
 *   Note: The address of the in-place "struct kfifo" object must be
 *   passed as the first argument to this functions
 * - Replace the use of __kfifo_put into kfifo_in and __kfifo_get
 *   into kfifo_out
 * - Replace the use of kfifo_put into kfifo_in_spinlocked and kfifo_get
 *   into kfifo_out_spinlocked
 *   Note: the spinlock pointer formerly passed to kfifo_init/kfifo_alloc
 *   must be passed now to the kfifo_in_spinlocked and kfifo_out_spinlocked
 *   as the last parameter
 * - The formerly __kfifo_* functions are renamed into kfifo_*
 */

/*
 * Note about locking: There is no locking required until only one reader
 * and one writer is using the fifo and no kfifo_reset() will be called.
 * kfifo_reset_out() can be safely used, until it will be only called
 * in the reader thread.
 * For multiple writer and one reader there is only a need to lock the writer.
 * And vice versa for only one writer and multiple reader there is only a need
 * to lock the reader.
 */

/*
 * 改编自内核版本v6.10
 * https://github.com/torvalds/linux/blob/v6.10/include/linux/kfifo.h
 * 1. 去除esize
 * 2. 数组存储指针类型，每个指针是一个record
 */

struct __kfifo {
	uint32_t	in;
	uint32_t	out;
	uint32_t	mask;
	const void*	    *data;
};

/**
 * INIT_KFIFO - Initialize a fifo declared by DECLARE_KFIFO
 * @fifo: name of the declared fifo datatype
 */
#define INIT_KFIFO(fifo) \
(void)({ \
	fifo.in = 0; \
	fifo.out = 0; \
	fifo.mask =  0; \
	fifo.data =  NULL; \
})

/**
 * DEFINE_KFIFO - macro to define and initialize a fifo
 * @fifo: name of the declared fifo datatype
 * @type: type of the fifo elements
 * @size: the number of elements in the fifo, this must be a power of 2
 *
 * Note: the macro can be used for global and local fifo data type variables.
 */
#define DEFINE_KFIFO(fifo) \
	struct __kfifo fifo = \
	{ \
		.in	= 0, \
		.out	= 0, \
		.mask	= 0, \
		.data	=  NULL, \
	}


/**
 * kfifo_initialized - Check if the fifo is initialized
 * @fifo: address of the fifo to check
 *
 * Return %true if fifo is initialized, otherwise %false.
 * Assumes the fifo was 0 before.
 */
#define kfifo_initialized(fifo) (fifo.mask)

/**
 * kfifo_esize - returns the size of the element managed by the fifo
 * @fifo: address of the fifo to be used
 */
#define kfifo_esize(fifo)	(fifo.esize)

/**
 * kfifo_size - returns the size of the fifo in elements
 * @fifo: address of the fifo to be used
 */
#define kfifo_size(fifo)	(fifo.mask + 1)

/**
 * kfifo_reset - removes the entire fifo content
 * @fifo: address of the fifo to be used
 *
 * Note: usage of kfifo_reset() is dangerous. It should be only called when the
 * fifo is exclusived locked or when it is secured that no other thread is
 * accessing the fifo.
 */
#define kfifo_reset(fifo) \
(void)({ \
	fifo.in = fifo.out = 0; \
})

/**
 * kfifo_reset_out - skip fifo content
 * @fifo: address of the fifo to be used
 *
 * Note: The usage of kfifo_reset_out() is safe until it will be only called
 * from the reader thread and there is only one concurrent reader. Otherwise
 * it is dangerous and must be handled in the same way as kfifo_reset().
 */
#define kfifo_reset_out(fifo)	\
(void)({ \
	fifo.out = fifo.in; \
})

/**
 * kfifo_len - returns the number of used elements in the fifo
 * @fifo: address of the fifo to be used
 */
#define kfifo_len(fifo) \
({ \
	fifo.in - fifo.out; \
})

/**
 * kfifo_is_empty - returns true if the fifo is empty
 * @fifo: address of the fifo to be used
 */
#define	kfifo_is_empty(fifo) \
({ \
	(fifo).in == (fifo).out; \
})

/**
 * kfifo_is_full - returns true if the fifo is full
 * @fifo: address of the fifo to be used
 */
#define	kfifo_is_full(fifo) \
({ \
	kfifo_len(fifo) > fifo.mask; \
})

/**
 * kfifo_avail - returns the number of unused elements in the fifo
 * @fifo: address of the fifo to be used
 */
#define	kfifo_avail(fifo) \
({ \
	kfifo_size(fifo) - kfifo_len(fifo); \
}) 

/**
 * kfifo_skip_count - skip output data
 * @fifo: address of the fifo to be used
 * @count: count of data to skip
 */
#define	kfifo_skip_count(fifo, count) do { \
	fifo->out += (count); \
} while(0)

/**
 * kfifo_skip - skip output data
 * @fifo: address of the fifo to be used
 */
#define	kfifo_skip(fifo)	kfifo_skip_count(fifo, 1)

extern int __kfifo_alloc(struct __kfifo *fifo, uint32_t size);

extern void __kfifo_free(struct __kfifo *fifo);

extern bool __kfifo_in(struct __kfifo *fifo, const void *val);

extern bool __kfifo_out(struct __kfifo *fifo, const void **val);

extern bool __kfifo_out_peek(struct __kfifo *fifo, const void **val);

#include "con_queue.h"

class KFifo : public ConQueue
{
private:
	struct __kfifo fifo_;
public:

	KFifo(uint32_t size);

	~KFifo() override;

	bool enqueue(const void *val) override;

	bool dequeue(const void **val) override;
};
