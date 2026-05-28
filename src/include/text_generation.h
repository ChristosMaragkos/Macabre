#include "sharpie.h"

#ifndef TEXT_GENERATION_H
#define TEXT_GENERATION_H

typedef struct {
    char *str;
    size_t length;
} StringView;

void find_last_word(const char *str, size_t length, StringView *out_view);

#endif
