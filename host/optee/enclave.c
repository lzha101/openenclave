#include <stdint.h>
#include <openenclave/host.h>
#include <openenclave/edger8r/host.h>
#include <stdlib.h>
#include <openenclave/internal/raise.h>
#include "enclave.h"
#include "optee_fake.h"

oe_result_t oe_create_enclave(
    const char* enclave_path,
    oe_enclave_type_t enclave_type,
    uint32_t flags,
    const void* config,
    uint32_t config_size,
    const oe_ocall_func_t* ocall_table,
    uint32_t ocall_table_size,
    oe_enclave_t** enclave_out)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_enclave_t* enclave = NULL;
    TEEC_Result res;
    uint32_t err_origin;

    /* Check parameters. */
    if (!enclave_path || !enclave_out || enclave_type != OE_ENCLAVE_TYPE_SGX ||
        (flags & OE_ENCLAVE_FLAG_RESERVED) || config || config_size > 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Allocate and zero-fill the enclave structure. */
    if (!(enclave = (oe_enclave_t*)calloc(1, sizeof(oe_enclave_t))))
        OE_RAISE(OE_OUT_OF_MEMORY);


    /***** Mock TZ Optee Code here. *****/
    res = TEEC_InitializeContext(NULL, &enclave->ctx);
    if (res != TEEC_SUCCESS)
        OE_RAISE(OE_FAILURE);

    res = TEEC_OpenSession(
        &enclave->ctx,
        &enclave->session,
        enclave_path,
        TEEC_LOGIN_PUBLIC,
        NULL,
        NULL,
        &err_origin);
    
    if (res != TEEC_SUCCESS)
        OE_RAISE(OE_FAILURE);
    /***** End Mock TZ Optee Code. *****/


    enclave->ocall_table = ocall_table;
    enclave->ocall_table_size = ocall_table_size;
    *enclave_out = enclave; 
    result = OE_OK;
    enclave = NULL;

done:
    if (enclave != NULL)
        oe_terminate_enclave(enclave);

    return result;
}

oe_result_t oe_call_enclave_function(
    oe_enclave_t* enclave,
    uint32_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    TEEC_Result res;
    TEEC_Operation op;
    char dummyInOutBuffer[1];
    uint32_t err_origin;

    if (input_buffer == NULL) {
        input_buffer = dummyInOutBuffer;
        input_buffer_size = 1;
    }

    if (output_buffer == NULL) {
        output_buffer = dummyInOutBuffer;
        output_buffer_size = 1;
    }

    op.in = input_buffer;
    op.insize = input_buffer_size;
    op.out = output_buffer;
    op.outsize = output_buffer_size;

    res = TEEC_InvokeCommand(&enclave->session, function_id, &op, &err_origin);
    if (output_bytes_written)
        *output_bytes_written = output_buffer_size;

    return res == TEEC_SUCCESS ? OE_OK : OE_UNEXPECTED;
}

oe_result_t oe_terminate_enclave(oe_enclave_t* enclave)
{
    if (enclave)
    {
        TEEC_CloseSession(&enclave->session);
        TEEC_FinalizeContext(&enclave->ctx);
        free(enclave);
    }

    return OE_OK;
}
