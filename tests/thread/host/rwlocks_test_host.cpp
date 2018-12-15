// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/error.h>
#include <openenclave/internal/tests.h>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "../rwlock_tests.h"
#include "thread_u.h"

void host_usleep(size_t microseconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

void* reader_thread(
    oe_enclave_t* enclave,
    size_t* readers,
    size_t* writers,
    size_t* max_readers,
    bool* readers_and_writers)
{
    OE_TEST(
        enc_reader_thread_impl(
            enclave, &readers, &writers, &max_readers, &readers_and_writers) ==
        OE_OK);

    return NULL;
}

void* writer_thread(
    oe_enclave_t* enclave,
    size_t* readers,
    size_t* writers,
    size_t* max_writers,
    bool* readers_and_writers)
{
    OE_TEST(
        enc_writer_thread_impl(
            enclave, &readers, &writers, &max_writers, &readers_and_writers) ==
        OE_OK);

    return NULL;
}

// Launch multiple reader and writer threads and OE_TEST invariants.
void test_readers_writer_lock(oe_enclave_t* enclave)
{
    std::thread threads[NUM_RW_TEST_THREADS];

    size_t readers = 0;
    size_t writers = 0;
    size_t max_readers = 0;
    size_t max_writers = 0;
    bool readers_and_writers = false;

    for (size_t i = 0; i < NUM_RW_TEST_THREADS; i++)
    {
        if (i & 1)
        {
            threads[i] = std::thread(
                writer_thread,
                enclave,
                &readers,
                &writers,
                &max_writers,
                &readers_and_writers);
        }
        else
        {
            threads[i] = std::thread(
                reader_thread,
                enclave,
                &readers,
                &writers,
                &max_readers,
                &readers_and_writers);
        }
    }

    for (size_t i = 0; i < NUM_RW_TEST_THREADS; i++)
    {
        threads[i].join();
    }

    // There can be at most 1 writer thread active.
    OE_TEST(max_writers == 1);

    // There can be at most NUM_THREADS/2 reader threads active
    // and no thread was starved.
    OE_TEST(max_readers <= NUM_READER_THREADS);

    // Readers and writer threads should never be simultaneously active.
    OE_TEST(readers_and_writers == false);

    // Additionally, the test requires that all readers are
    // simultaneously active at least once.
    OE_TEST(max_readers == NUM_READER_THREADS);
}
