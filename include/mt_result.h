#pragma once
#include <stdbool.h>
#include "mt_err.h"

typedef struct {
    bool ok;
    union {
        void    *value;
        mt_err_t err;
    } u;
} mt_result_t;

#define RESULT_OK(v)   ((mt_result_t){ .ok = true,  .u.value = (v) })
#define RESULT_ERR(e)  ((mt_result_t){ .ok = false, .u.err   = (e) })
#define MT_RETURN_OK(v) return RESULT_OK(v)
#define MT_RETURN_ERR(e) return RESULT_ERR(e)

