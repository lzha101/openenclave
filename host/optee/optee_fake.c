#include "optee_fake.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <openenclave/bits/defs.h>
#include <signal.h>
#include <string.h>

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
    int child_stdin[2];
    int child_stdout[2];
    pid_t pid;
   
    pipe(child_stdin);
    pipe(child_stdout);
   
    pid = fork();
    if (pid < 0)
        return -1;

    if (pid > 0)
    {
        // parent
        close(child_stdin[0]);
        close(child_stdout[1]);

        session->pid = pid;
        session->child_stdin = child_stdin[1];
        session->child_stdout = child_stdout[0];
        return 0;
    }

    // child, no return
    close(child_stdin[1]);
    close(child_stdout[0]);
    dup2(child_stdin[0], 0);
    dup2(child_stdout[1], 1);
    close(child_stdin[0]);
    close(child_stdin[1]);
    execl(dest, dest, NULL);
    return -1;
}

TEEC_Result TEEC_InvokeCommand(
    TEEC_Session* session,
    uint32_t commandID,
    TEEC_Operation* operation,
    uint32_t* returnOrigin)
{
    OE_UNUSED(returnOrigin);
    size_t out;

    // Write [command, insize, in]
    write(session->child_stdin, &commandID, sizeof(commandID));
    write(session->child_stdin, &operation->insize, sizeof(operation->insize)); 
    write(session->child_stdin, operation->in, operation->insize);

    // Wait for response. [outsize, out]
    read(session->child_stdout, &out, sizeof(out));
    read(session->child_stdout, operation->out, out);

    return TEEC_SUCCESS;
}

void TEEC_CloseSession(TEEC_Session* session)
{
    if (session->pid != 0)
    {
        close(session->child_stdin);
        close(session->child_stdout);
        kill(session->pid, SIGKILL);
        memset(session, 0, sizeof(*session));
    }
}

void TEEC_FinalizeContext(TEEC_Context* context)
{
    OE_UNUSED(context);
}

