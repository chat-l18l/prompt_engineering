#include <stdio.h>
#include "mt_string_view.h"

static void print_sv(const char *label, mt_sv_t sv) {
    printf("%s: '%.*s' (len=%zu)\n", label, (int)sv.len, sv.data ? sv.data : "", sv.len);
}

int main(void) {
    mt_sv_t line = mt_sv_from_cstr("  host=example.com;port=8000  ");
    line = mt_sv_trim(line);
    print_sv("trimmed", line);

    mt_sv_t iter = line;
    while (iter.len > 0) {
        mt_result_sv_t kv = mt_sv_chop_by_delim(&iter, ';');
        mt_sv_t field = kv.ok ? kv.u.value : iter;
        if (!kv.ok) {
            iter = mt_sv_from_parts(NULL, 0);
        }

        mt_result_sv_pair_t split = mt_sv_split_first(field, '=');
        if (!split.ok) {
            printf("skip malformed field (%s)\n", mt_err_str(split.u.err));
            continue;
        }

        mt_sv_t key = mt_sv_trim(split.u.value.head);
        mt_sv_t val = mt_sv_trim(split.u.value.tail);
        print_sv("key", key);
        print_sv("val", val);
    }

    return 0;
}
