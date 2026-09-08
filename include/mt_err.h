#pragma once
#include <stdint.h>

// Generic error codes. All codes are positive; 0 equals success.
typedef enum {
    MT_OK = 0,
    MT_ERR_NULL = 1,
    MT_ERR_OOM = 2,
    MT_ERR_IO = 3,
    MT_ERR_RANGE = 4,
    MT_ERR_PARSE = 5,
    MT_ERR_BUSY = 6,
    MT_ERR_TIMEOUT = 7,
    MT_ERR_NOTFOUND = 8,
    MT_ERR_PERM = 9,

    MT_ERR_CUSTOM = 1000,
    MT_ERR_SV_CHOP = MT_ERR_CUSTOM,
    MT_ERR_SV_EMPTY,
} mt_err_t;

static inline const char *mt_err_str(mt_err_t err) {
    switch (err) {
    case MT_OK: return "MT_OK";
    case MT_ERR_NULL: return "MT_ERR_NULL";
    case MT_ERR_OOM: return "MT_ERR_OOM";
    case MT_ERR_IO: return "MT_ERR_IO";
    case MT_ERR_RANGE: return "MT_ERR_RANGE";
    case MT_ERR_PARSE: return "MT_ERR_PARSE";
    case MT_ERR_BUSY: return "MT_ERR_BUSY";
    case MT_ERR_TIMEOUT: return "MT_ERR_TIMEOUT";
    case MT_ERR_NOTFOUND: return "MT_ERR_NOTFOUND";
    case MT_ERR_PERM: return "MT_ERR_PERM";
    case MT_ERR_SV_CHOP: return "MT_ERR_SV_CHOP";
    case MT_ERR_SV_EMPTY: return "MT_ERR_SV_EMPTY";
    default: return "MT_ERR_UNKNOWN";
    }
}
