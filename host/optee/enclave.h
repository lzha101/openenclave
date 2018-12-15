#include <stdint.h>
#include <openenclave/host.h>
#include "optee_fake.h"

struct _oe_enclave
{
    TEEC_Context ctx;
    TEEC_Session session;
    const oe_ocall_func_t* ocall_table;
    uint32_t ocall_table_size;
};
