#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#ifndef _HOST_OPTEE_FAKE_OPTEE_
#define _HOST_OPTEE_FAKE_OPTEE_

typedef int TEEC_Result;
typedef int TEEC_Context;
typedef pid_t TEEC_Session;
typedef char TEEC_UUID;

typedef struct _TEEC_Operation {
    const void* in;
    size_t insize;
    void* out;
    size_t outsize;
} TEEC_Operation;

#define TEEC_SUCCESS 0
#define TEEC_LOGIN_PUBLIC 0

TEEC_Result TEEC_InitializeContext(const char* name, TEEC_Context* context);

TEEC_Result TEEC_OpenSession(
    TEEC_Context* context,
    TEEC_Session* session,
    const TEEC_UUID* dest,
    uint32_t connectionMethod,
    const void* connectionData,
    TEEC_Operation* operation,
    uint32_t* returnOrigin);

TEEC_Result TEEC_InvokeCommand(
    TEEC_Session* session,
    uint32_t commandID,
    TEEC_Operation* operation,
    uint32_t* returnOrigin);

void TEEC_CloseSession(TEEC_Session* session);
void TEEC_FinalizeContext(TEEC_Context* context);

#endif 
