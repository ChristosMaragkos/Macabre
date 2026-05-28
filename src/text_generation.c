#include "include/text_generation.h"
#include "include/sharpie.h"

void find_last_word(const char *str, size_t length, StringView *out_view) {
    if (length == 0) {
        out_view->str = NULL;
        out_view->length = 0;
        return;
    }

    int i = length - 1;
    while (i >= 0 && str[i] == ' ')
        --i;

    if (i < 0) {
        out_view->str = NULL;
        out_view->length = 0;
        return;
    }

    int end = i;
    while (i >= 0 && str[i] != ' ')
        --i;

    int start = i + 1;

    out_view->str = &str[start];
    out_view->length = (size_t)(end - i);
}
