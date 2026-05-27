#include "include/text_rendering.h"
#include "include/macabre_table.h"
#include "include/sharpie.h"

static const unsigned int TEXT_X_STARTING = 8;
static const unsigned int TEXT_Y_STARTING = 32;
static const unsigned int CAMERA_Y_STARTING = 0;

// Current pen position in world space (not screen space)
// Starts at top-left of conversation area
static unsigned int text_cursor_x = TEXT_X_STARTING;
static unsigned int text_cursor_y = TEXT_Y_STARTING;
static unsigned int camera_y = CAMERA_Y_STARTING;

void emit_char(unsigned char char_idx) {
    if (char_idx == MACABRE_NEWLINE_IDX) {
        text_cursor_x = TEXT_LEFT_MARGIN;
        text_cursor_y += TEXT_LINE_HEIGHT;
    } else if (char_idx == MACABRE_SPACE_IDX) {
        text_cursor_x += 8;
        if (text_cursor_x >= TEXT_RIGHT_MARGIN) {
            text_cursor_x = TEXT_LEFT_MARGIN;
            text_cursor_y += TEXT_LINE_HEIGHT;
        }
    } else {
        draw_sprite(text_cursor_x, text_cursor_y, char_idx, 0, 0);
        text_cursor_x += 8;
        if (text_cursor_x >= TEXT_RIGHT_MARGIN) {
            text_cursor_x = TEXT_LEFT_MARGIN;
            text_cursor_y += TEXT_LINE_HEIGHT;
        }
    }
}

void emit_string(const char *str, size_t length) {
    text_cursor_x = TEXT_X_STARTING;
    text_cursor_y = TEXT_Y_STARTING;
    camera_y = CAMERA_Y_STARTING;

    for (int i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)*(str + i);
        unsigned char idx;

        if (ch >= 'A' && ch <= 'Z') {
            idx = ch - 'A'; // 0-25
        } else if (ch >= '0' && ch <= '9') {
            idx = ch - '0' + 26; // 26-35
        } else if (ch == ' ') {
            idx = 36;
        } else {
            // scan the charset for punctuation
            idx = ascii_to_sprite(ch);
        }

        emit_char(idx);
    }
}
