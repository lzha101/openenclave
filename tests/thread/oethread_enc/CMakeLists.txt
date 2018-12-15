# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(../thread.edl enclave gen)

add_executable(oethread_enc
    enc.cpp
    cond_tests.cpp
    rwlock_tests.cpp
    ${gen})

target_include_directories(oethread_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(oethread_enc oelibcxx oeenclave)
