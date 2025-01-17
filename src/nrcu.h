#ifndef NRCU_H
#define NRCU_H

#include <stdatomic.h>

// TODO: should we turn these into opaque types?
//    by extension, to allow this; maybe enforce nrcu_promise_t to only be handled as a ptr
typedef struct {
    atomic_uint generation;
    atomic_uint readers[2];
} nrcu_context_t;

typedef atomic_uint nrcu_promise_t;

// reader API
nrcu_promise_t nrcu_read_lock(nrcu_context_t *ctx);
void nrcu_read_unlock(nrcu_context_t *ctx, nrcu_promise_t promise);

// writer API
void nrcu_synchronize(nrcu_context_t *ctx);

#endif // NRCU_H
