#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "mt_err.h"
#include "mt_result.h"

typedef struct {
    const char *data;
    size_t      len;
} mt_sv_t;

typedef struct {
    bool ok;
    union {
        mt_sv_t  value;
        mt_err_t err;
    } u;
} mt_result_sv_t;

typedef struct {
    mt_sv_t head;
    mt_sv_t tail;
} mt_sv_pair_t;

typedef struct {
    bool ok;
    union {
        mt_sv_pair_t value;
        mt_err_t     err;
    } u;
} mt_result_sv_pair_t;

#define MT_OK_SV(v)      ((mt_result_sv_t){ .ok = true,  .u.value = (v) })
#define MT_ERR_SV(e)     ((mt_result_sv_t){ .ok = false, .u.err   = (e) })
#define MT_OK_SV_PAIR(v) ((mt_result_sv_pair_t){ .ok = true,  .u.value = (v) })
#define MT_ERR_SV_PAIR(e)((mt_result_sv_pair_t){ .ok = false, .u.err   = (e) })
#define MT_RETURN_OK_SV(v)      return MT_OK_SV(v)
#define MT_RETURN_ERR_SV(e)     return MT_ERR_SV(e)
#define MT_RETURN_OK_SV_PAIR(v) return MT_OK_SV_PAIR(v)
#define MT_RETURN_ERR_SV_PAIR(e) return MT_ERR_SV_PAIR(e)

mt_sv_t mt_sv_from_parts(const char *data, size_t len);
mt_sv_t mt_sv_from_cstr(const char *cstr);
mt_sv_t mt_sv_trim(mt_sv_t sv);
bool    mt_sv_equals(mt_sv_t a, mt_sv_t b);
bool    mt_sv_starts_with(mt_sv_t sv, mt_sv_t prefix);
mt_result_sv_t mt_sv_chop_left(mt_sv_t *sv, size_t count);
mt_result_sv_t mt_sv_chop_by_delim(mt_sv_t *sv, char delim);
mt_result_sv_pair_t mt_sv_split_first(mt_sv_t sv, char delim);
