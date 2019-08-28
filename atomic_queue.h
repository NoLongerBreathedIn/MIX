/*
 * atomic_queue.h
 *
 *  Created on: Aug 5, 2019
 *      Author: eyal
 */

#ifndef ATOMIC_QUEUE_H_
#define ATOMIC_QUEUE_H_

#include <stdbool.h>

#ifdef CDT_PARSER
#define _Atomic
typedef char atomic_flag;
#else
#include <stdatomic.h>
#endif

struct aq;

typedef struct aq atomic_queue;

extern atomic_queue *create_atomic_queue();
// Returns true if destruction was successful.
extern bool destroy_atomic_queue(atomic_queue *q);
extern void atomic_enqueue(atomic_queue *q, void *obj, atomic_flag *elt_lock);

/* Acquires the flag associated to the element it returns. */
extern void *atomic_dequeue(atomic_queue *q);
extern bool atomic_queue_is_empty(atomic_queue *q);

#endif /* ATOMIC_QUEUE_H_ */
