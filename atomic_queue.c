/*
 * atomic_queue.c
 *
 *  Created on: Aug 5, 2019
 *      Author: eyal
 */


#include "atomic_queue.h"
#include <pthread.h>
#include <stdlib.h>

#define P_SEM(x) while(atomic_flag_test_and_set(&x)) sched_yield()
#define V_SEM(x) atomic_flag_clear(&x)

struct qelt {
  void *data;
  atomic_flag *elt_usable;
  struct qelt *next;
};

struct aq {
  struct qelt *begin;
  struct qelt **end;
  atomic_flag lock;
};


atomic_queue *create_atomic_queue() {
  atomic_queue *r = malloc(sizeof(atomic_queue));
  r->begin = NULL;
  r->end = &r->begin;
  r->lock = (atomic_flag)ATOMIC_FLAG_INIT;
  return(r);
}

bool destroy_atomic_queue(atomic_queue *q) {
  P_SEM(q->lock);
  if(q->begin == NULL) {
    free(q);
    return(true);
  }
  V_SEM(q->lock);
  return(false);
}

void atomic_enqueue(atomic_queue *q, void *obj, atomic_flag *is_usable) {
  P_SEM(q->lock);
  struct qelt *next = malloc(sizeof(struct qelt));
  next->data = obj;
  next->elt_usable = is_usable;
  next->next = NULL;
  if(q->begin == NULL)
    q->begin = next;
  else 
    *q->end = next;
  q->end = &next->next;
  V_SEM(q->lock);
}

void *atomic_dequeue(atomic_queue *q) {
  P_SEM(q->lock);
  struct qelt **link = &q->begin;
  /* Continue, skipping locked elements until get to first usable element */
  while(*link != NULL && (*link)->elt_usable != NULL &&
	atomic_flag_test_and_set((*link)->elt_usable))
    link = &(*link)->next;
  void *ret = NULL;
  /* If x usable, rewire pointers:
     a) *link (formerly ptr to x) now goes to x->next.
     b) If that's null, then q->end == &x->next,
     so point q->end at *link instead.
  */
  if(link != NULL) {
    struct qelt *elt = *link;
    ret = elt->data;	
    if((*link = elt->next) == NULL)
      q->end = link;
    free(elt);
  }
  V_SEM(q->lock);
  return(ret);
}

bool atomic_queue_is_empty(atomic_queue *q) {
  bool ret;
  P_SEM(q->lock);
  ret = q->begin == NULL;
  V_SEM(q->lock);
  return(ret);
}
