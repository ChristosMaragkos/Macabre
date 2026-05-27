#include "sharpie.h"

#ifndef TEXT_RENDERING_H
#define TEXT_RENDERING_H

#pragma once

#define TEXT_LEFT_MARGIN 8
#define TEXT_RIGHT_MARGIN 248
#define TEXT_LINE_HEIGHT 9
#define TEXT_AREA_BOTTOM 200

void emit_char(unsigned char char_idx);
void emit_string(const char *str, size_t len);

#endif
