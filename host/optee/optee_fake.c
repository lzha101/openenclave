#include "optee_fake.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <openenclave/bits/defs.h>
#include <signal.h>

TEEC_Result TEEC_InitializeContext(const char* name, TEEC_Context* context)
{
    OE_UNUSED(name);
    OE_UNUSED(context);
    return TEEC_SUCCESS;
}

TEEC_Result TEEC_OpenSession(
    TEEC_Context* context,
    TEEC_Session* session,
    const TEEC_UUID* dest,
    uint32_t connectionMethod,
    const void* connectionData,
    TEEC_Operation* operation,
    uint32_t* returnOrigin)
{
    OE_UNUSED(context);
    OE_UNUSED(connectionMethod);
    OE_UNUSED(connectionData);
    OE_UNUSED(operation);
    OE_UNUSED(returnOrigin);

    // fake with forkexec
    pid_t pid = fork();
    if (pid < 0)
        return -1;

    if (pid > 0)
    {
        // parent
        *session = pid;
        return 0;
    }

    // child, no return;
    execl(dest, dest, NULL);
    return -1;
}

TEEC_Result TEEC_InvokeCommand(
    TEEC_Session* session,
    uint32_t commandID,
    TEEC_Operation* operation,
    uint32_t* returnOrigin)
{
    OE_UNUSED(session);
    OE_UNUSED(commandID);
    OE_UNUSED(operation);
    OE_UNUSED(returnOrigin);
    return TEEC_SUCCESS;
}

void TEEC_CloseSession(TEEC_Session* session)
{
    if (*session != 0)
        kill(*session, SIGKILL);
}

void TEEC_FinalizeContext(TEEC_Context* context)
{
    OE_UNUSED(context);
}

