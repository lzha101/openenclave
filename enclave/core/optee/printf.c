#include <openenclave/internal/trace.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/types.h>
#include <openenclave/bits/defs.h>

oe_result_t oe_log(log_level_t level, const char* fmt, ...)
{
    oe_va_list ap;
    oe_va_start(ap, fmt);
    oe_host_fprintf(0, fmt, ap);
    oe_va_end(ap);
    return OE_OK;
}

int oe_host_fprintf(int device, const char* fmt, ...)
{
    OE_UNUSED(device);
    OE_UNUSED(fmt);
    return -1;
}
