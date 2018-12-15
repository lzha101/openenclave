#include <openenclave/internal/trace.h>
#include <stdio.h>
#include <stdarg.h>

void oe_log(log_level_t level, const char* fmt, ...)
{
    OE_UNUSED(level);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
