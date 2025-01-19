#include <stdio.h>
#include <time.h>
#include "nrcu.h"

nrcu_promise_t nrcu_read_lock(nrcu_context_t *ctx)
{
    // this is probably overkill, we don't care which generation we get as long as we inc the same one
    // same goes for a lot of the other fences, but meh
    //atomic_thread_fence(memory_order_seq_cst);

    nrcu_promise_t promise = atomic_load(&ctx->generation);
    atomic_fetch_add_explicit(&ctx->readers[promise & 1], 1, memory_order_acquire);

    atomic_thread_fence(memory_order_seq_cst);

    return promise;
}

void nrcu_read_unlock(nrcu_context_t *ctx, nrcu_promise_t promise)
{
    atomic_thread_fence(memory_order_seq_cst);
    atomic_fetch_sub_explicit(&ctx->readers[promise & 1], 1, memory_order_release);
}

void nrcu_synchronize(nrcu_context_t *ctx)
{
    atomic_thread_fence(memory_order_seq_cst);
    // we want to reduce congestion, and we are the only writer, no fancy inc stuff
    // TODO: is the previous line dumb??
    nrcu_promise_t promise = atomic_load(&ctx->generation);
    atomic_store(&ctx->generation, promise + 1);

    clock_t start = clock();

    while (atomic_load_explicit(&ctx->readers[promise & 1], memory_order_acquire)) {
        // Could add small delay here if needed
    }

    // printf("took %lu\n", clock() - start);

    atomic_thread_fence(memory_order_seq_cst);
}
