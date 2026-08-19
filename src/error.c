#include "error.h"
#include "system.h"

static uint32_t err_reg;
static uint32_t err_time[ERR_MAX];

void error_assert(error_t err)
{
    if (err < ERR_MAX) {
        err_time[err] = system_millis();
        err_reg |= (1UL << err);
    }
}

uint32_t error_timestamp(error_t err)
{
    return err < ERR_MAX ? err_time[err] : 0U;
}

uint8_t error_occurred(error_t err)
{
    return err < ERR_MAX && (err_reg & (1UL << err)) != 0U;
}

uint32_t error_reg(void) { return err_reg; }
