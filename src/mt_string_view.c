#include "mt_string_view.h"
#include <ctype.h>
#include <string.h>

mt_sv_t mt_sv_from_parts(const char *data, size_t len) {
    mt_sv_t sv = {0};
    sv.data = data;
    sv.len = data ? len : 0;
    return sv;
}

mt_sv_t mt_sv_from_cstr(const char *cstr) {
    if (!cstr) {
        mt_sv_t empty = {0};
        return empty;
    }
    return mt_sv_from_parts(cstr, strlen(cstr));
}

mt_sv_t mt_sv_trim(mt_sv_t sv) {
    while (sv.len > 0 && isspace((unsigned char)sv.data[0])) {
        sv.data += 1;
        sv.len  -= 1;
    }
    while (sv.len > 0 && isspace((unsigned char)sv.data[sv.len - 1])) {
        sv.len -= 1;
    }
    return sv;
}

bool mt_sv_equals(mt_sv_t a, mt_sv_t b) {
    if (a.len != b.len) {
        return false;
    }
    if (a.len == 0) {
        return true;
    }
    return memcmp(a.data, b.data, a.len) == 0;
}

bool mt_sv_starts_with(mt_sv_t sv, mt_sv_t prefix) {
    if (sv.len < prefix.len) {
        return false;
    }
    if (prefix.len == 0) {
        return true;
    }
    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

mt_result_sv_t mt_sv_chop_left(mt_sv_t *sv, size_t count) {
    if (!sv) {
        MT_RETURN_ERR_SV(MT_ERR_NULL);
    }
    if (count == 0) {
        MT_RETURN_ERR_SV(MT_ERR_SV_EMPTY);
    }
    if (count > sv->len) {
        MT_RETURN_ERR_SV(MT_ERR_RANGE);
    }

    mt_sv_t left = mt_sv_from_parts(sv->data, count);
    sv->data += count;
    sv->len  -= count;
    MT_RETURN_OK_SV(left);
}

mt_result_sv_t mt_sv_chop_by_delim(mt_sv_t *sv, char delim) {
    if (!sv) {
        MT_RETURN_ERR_SV(MT_ERR_NULL);
    }
    if (!sv->data || sv->len == 0) {
        MT_RETURN_ERR_SV(MT_ERR_SV_EMPTY);
    }

    for (size_t i = 0; i < sv->len; ++i) {
        if (sv->data[i] == delim) {
            mt_sv_t left = mt_sv_from_parts(sv->data, i);
            sv->data += i + 1;
            sv->len  -= i + 1;
            MT_RETURN_OK_SV(left);
        }
    }

    MT_RETURN_ERR_SV(MT_ERR_NOTFOUND);
}

mt_result_sv_pair_t mt_sv_split_first(mt_sv_t sv, char delim) {
    if (!sv.data && sv.len > 0) {
        MT_RETURN_ERR_SV_PAIR(MT_ERR_NULL);
    }
    for (size_t i = 0; i < sv.len; ++i) {
        if (sv.data[i] == delim) {
            mt_sv_pair_t pair = {
                .head = mt_sv_from_parts(sv.data, i),
                .tail = mt_sv_from_parts(sv.data + i + 1, sv.len - i - 1),
            };
            MT_RETURN_OK_SV_PAIR(pair);
        }
    }

    MT_RETURN_ERR_SV_PAIR(MT_ERR_NOTFOUND);
}
