/*
 * Simple linux UM x86-64 use example
 */

#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

#include "nrcu.h"


char *g_nrcu_foo_data = NULL;

nrcu_context_t g_nrcu_foo_context;


void *reader_thread(void *arg)
{
    bool run = true;
    volatile char *current_data;
    unsigned long long ticks = 0;

    while (run) {
        ticks += 1;
        // read start
        const volatile nrcu_promise_t promise = nrcu_read_lock(&g_nrcu_foo_context);

        current_data = atomic_load(&g_nrcu_foo_data);\
        // mimic work before we end, realistically you wouldn't ever sleep in a reader
        // sched_yield();
        usleep(2000);

        switch (*current_data) {
            case '0':
                run = false;
                goto read_end;
            case '1':
                goto read_end;
            default:
                printf("what the fuck???\n");
                break;
        }

    read_end:
        nrcu_read_unlock(&g_nrcu_foo_context, promise);
        // read end
        usleep(700);
    }

    printf("stopping after %llu ticks\n", ticks);

    return NULL;
}


char g_data_0[] = "0";
char g_data_1[] = "1";


int main(int argc, char *argv[])
{
    printf("hello world\n");

    g_nrcu_foo_data  = g_data_1;

    pthread_t threads[10] = {0};

    for (int i = 0; i < sizeof(threads) / sizeof(threads[0]); i++) {
        if (0 != pthread_create(&threads[i], NULL, reader_thread, NULL)) {
            return 1;
        }
    }

    printf("spawned a bunch of threads\n");

    // in a normal RCU scenario we might wish to free(3) the old data, but that is not responsive
    // enough for the test, as the memory will almost definitely not get munmap(2) in most loop iterations
    // so mimic a free using poison data, keep doing over and over until we see a bug; YES we leak memory here.
    int counter = 0;
    do {
        char *data = malloc(1);
        if (NULL == data) {
            abort();
        }
        data[0] = '1';

        // write start
        char *old = atomic_exchange(&g_nrcu_foo_data, data);
        nrcu_synchronize(&g_nrcu_foo_context);
        // write end
        old[0] = '2';
    } while (0 == usleep(2000));

    printf("stopping them...\n");
    g_nrcu_foo_data = g_data_0;

    for (int i = 0; i < sizeof(threads) / sizeof(threads[0]); i++) {
        pthread_join(threads[i], NULL);
    }

    printf("done joining!\n");

    return 0;
}
