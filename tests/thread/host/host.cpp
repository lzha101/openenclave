// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/error.h>
#include <openenclave/internal/tests.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include "../../../host/enclave.h"
#include "thread_u.h"

static std::atomic_flag _host_tcs_lock = ATOMIC_FLAG_INIT;

const size_t NUM_THREADS = 8;

static inline void _acquire_lock(std::atomic_flag* lock)
{
    while (lock->test_and_set(std::memory_order_acquire))
    {
        continue;
    }
}

static inline void _release_lock(std::atomic_flag* lock)
{
    lock->clear(std::memory_order_release);
}

void* test_mutex_thread(oe_enclave_t* enclave, size_t* count1, size_t* count2)
{
    oe_result_t result = enc_test_mutex(enclave, &count1, &count2);
    OE_TEST(result == OE_OK);

    return NULL;
}

void test_mutex(oe_enclave_t* enclave)
{
    size_t count1 = 0;
    size_t count2 = 0;
    std::thread threads[NUM_THREADS];

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = std::thread(test_mutex_thread, enclave, &count1, &count2);
    }

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }

    OE_TEST(count1 == NUM_THREADS);
    OE_TEST(count2 == NUM_THREADS);
}

void* waiter_thread(oe_enclave_t* enclave)
{
    oe_result_t result = enc_wait(enclave, NUM_THREADS);
    OE_TEST(result == OE_OK);

    return NULL;
}

void test_cond(oe_enclave_t* enclave)
{
    std::thread threads[NUM_THREADS];

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = std::thread(waiter_thread, enclave);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        OE_TEST(enc_signal(enclave) == OE_OK);
    }

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }
}

void* cb_test_waiter_thread(oe_enclave_t* enclave)
{
    OE_TEST(cb_test_waiter_thread_impl(enclave) == OE_OK);

    return NULL;
}

void* cb_test_signal_thread(oe_enclave_t* enclave)
{
    OE_TEST(cb_test_signal_thread_impl(enclave) == OE_OK);

    return NULL;
}

void test_cond_broadcast(oe_enclave_t* enclave)
{
    std::thread threads[NUM_THREADS];
    std::thread signal_thread;

    printf("test_cond_broadcast Starting\n");

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = std::thread(cb_test_waiter_thread, enclave);
    }

    signal_thread = std::thread(cb_test_signal_thread, enclave);

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }

    signal_thread.join();

    printf("test_cond_broadcast Complete\n");
}

void* exclusive_access_thread(oe_enclave_t* enclave)
{
    const size_t ITERS = 2;

    printf("exclusive_access_thread Starting\n");
    for (size_t i = 0; i < ITERS; i++)
    {
        OE_TEST(enc_wait_for_exclusive_access(enclave) == OE_OK);
        std::this_thread::sleep_for(std::chrono::microseconds(20 * 1000));

        OE_TEST(enc_relinquish_exclusive_access(enclave) == OE_OK);
        std::this_thread::sleep_for(std::chrono::microseconds(20 * 1000));
    }
    printf("exclusive_access_thread Ending\n");
    return NULL;
}

void test_thread_wake_wait(oe_enclave_t* enclave)
{
    std::thread threads[NUM_THREADS];

    printf("test_thread_wake_wait Starting\n");
    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = std::thread(exclusive_access_thread, enclave);
    }

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }

    // The oe_calls in this test should succeed without any segv/double free.
    printf("test_thread_wake_wait Complete\n");
}

void* lock_and_unlock_thread1(oe_enclave_t* enclave)
{
    const size_t ITERS = 20000;

    for (size_t i = 0; i < ITERS; ++i)
    {
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "ABC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "AB") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "AC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "BC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "ABBC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "ABAB") == OE_OK);
    }

    return NULL;
}

void* lock_and_unlock_thread2(oe_enclave_t* enclave)
{
    const size_t ITERS = 20000;

    for (size_t i = 0; i < ITERS; ++i)
    {
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "BC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "ABC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "BBCC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "BBC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "ABAB") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "ABAC") == OE_OK);
        OE_TEST(enc_lock_and_unlock_mutexes(enclave, "ABAB") == OE_OK);
    }

    return NULL;
}

// Launch multiple threads and try out various locking patterns on 3 mutexes.
// The locking patterns are chosen to not deadlock.
void test_thread_locking_patterns(oe_enclave_t* enclave)
{
    std::thread threads[NUM_THREADS];

    printf("test_thread_locking_patterns Starting\n");
    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = std::thread(
            (i & 1) ? lock_and_unlock_thread2 : lock_and_unlock_thread1,
            enclave);
    }

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }

    // The oe_calls in this test should succeed without any OE_TEST() failures.
    printf("test_thread_locking_patterns Complete\n");
}

void test_readers_writer_lock(oe_enclave_t* enclave);

void* tcs_thread(
    oe_enclave_t* enclave,
    size_t* num_tcs_used,
    size_t* num_out_threads,
    size_t tcs_req_count)
{
    oe_result_t result = enc_test_tcs_exhaustion(
        enclave, &num_tcs_used, &num_out_threads, tcs_req_count);
    if (result == OE_OUT_OF_THREADS)
    {
        _acquire_lock(&_host_tcs_lock);
        ++(*num_out_threads);
        _release_lock(&_host_tcs_lock);
    }
    else
        OE_TEST(result == OE_OK);

    return NULL;
}

// Thread binding test to verify TCS exhaustion i.e. enter on N threads when
// there are M TCSes where N > M; oe_call should return OE_OUT_OF_THREADS when
// M enclaves are in use.
// Trick is to keep the M enclaves busy until we get TCS exhaustion
void test_tcs_exhaustion(oe_enclave_t* enclave)
{
    std::vector<std::thread> threads;
    // Set the test_tcs_count to a value greater than the enclave TCSCount
    size_t test_tcs_req_count = enclave->num_bindings * 2;
    printf(
        "test_tcs_exhaustion - Number of TCS bindings in enclave=%zu\n",
        enclave->num_bindings);
    // Initialization of the shared variables before creating threads/launching
    // enclaves
    size_t num_tcs_used = 0;
    size_t num_out_threads = 0;
    size_t tcs_req_count = test_tcs_req_count;

    for (size_t i = 0; i < test_tcs_req_count; i++)
    {
        threads.push_back(
            std::thread(
                tcs_thread,
                enclave,
                &num_tcs_used,
                &num_out_threads,
                tcs_req_count));
    }

    for (size_t i = 0; i < test_tcs_req_count; i++)
    {
        threads[i].join();
    }

    printf(
        "test_tcs_exhaustion: tcs_count=%lu; num_threads=%lu; "
        "num_out_threads=%lu\n",
        static_cast<unsigned long>(test_tcs_req_count),
        static_cast<unsigned long>(num_tcs_used),
        static_cast<unsigned long>(num_out_threads));

    // Crux of the test is to get OE_OUT_OF_THREADS i.e. to exhaust the TCSes
    OE_TEST(num_out_threads > 0);
    // Verifying that everything adds up fine
    OE_TEST(num_tcs_used + num_out_threads == test_tcs_req_count);
    // Sanity test that we are not reusing the bindings
    OE_TEST(num_tcs_used <= enclave->num_bindings);
}

int main(int argc, const char* argv[])
{
    oe_result_t result;
    oe_enclave_t* enclave = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s ENCLAVE\n", argv[0]);
        exit(1);
    }

    const uint32_t flags = oe_get_create_flags();

    result = oe_create_thread_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        oe_put_err("oe_create_thread_enclave(): result=%u", result);
    }

    test_mutex(enclave);

    test_cond(enclave);

    test_cond_broadcast(enclave);

    test_thread_wake_wait(enclave);

    test_thread_locking_patterns(enclave);

    test_readers_writer_lock(enclave);

    test_tcs_exhaustion(enclave);

    if ((result = oe_terminate_enclave(enclave)) != OE_OK)
    {
        oe_put_err("oe_terminate_enclave(): result=%u", result);
    }

    printf("=== passed all tests (%s)\n", argv[0]);

    return 0;
}
