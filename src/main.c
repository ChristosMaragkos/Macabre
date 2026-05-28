#include "include/font.h"
#include "include/macabre_table.h"
#include "include/sharpie.h"
#include "include/text_rendering.h"

#define true 1
#define false 0
#define YES(b) ((b) == true)
#define NOT(b) ((b) == false)

static const unsigned char MIN_X = 32;
static const unsigned char MAX_X = 221;

static const unsigned char MIN_Y = 210;
static const unsigned char MAX_Y = 230;

static const unsigned char COLOR_BG = 14;

static const unsigned char LAST_CHAR_POS_X = 132;
static const unsigned char LAST_CHAR_POS_Y = 230;

// we reuse the same sprites, just flipped. Retro console dev, baby!
static const unsigned char SPR_CURSOR_RIGHT = FONT_SPRITE_COUNT - 3;
static const unsigned char SPR_CURSOR_LEFT = FONT_SPRITE_COUNT - 3;

static const unsigned char SPR_CURSOR_TOP = FONT_SPRITE_COUNT - 4;
static const unsigned char SPR_CURSOR_BOTTOM = FONT_SPRITE_COUNT - 4;

// How many frames can the cursor not be moved for,
// once it's moved once?
static const unsigned char CURSOR_INITIAL_DELAY = 8;

static const unsigned char APPEND_INITIAL_DELAY = 5;
static const unsigned char BACKSPACE_INITIAL_DELAY = 5;

static const unsigned char MAX_INPUT_LENGTH = 64;

static unsigned int oam_cursor_after_keyboard = 0;

static unsigned char cursor_moving = false;
static unsigned char cursor_move_delay = 0;

static Button current_frame_input = BTN_NONE;

static unsigned char selected_char_x = MIN_X;
static unsigned char selected_char_y = MIN_Y;

char input_buffer[64];
static unsigned char input_length = 0;

static unsigned char append_delay = 0;
static unsigned char backspace_delay = 0;

void draw_cursor(unsigned char x, unsigned char y);
unsigned char keyboard_pos_to_char_idx(unsigned char x, unsigned char y);
void input_append(unsigned char char_idx);
void input_backspace(void);

int main(void) {
    asm("ATTR 17");

    clear_screen(COLOR_BG);
    {
        unsigned char x = MIN_X;
        unsigned char y = MIN_Y;

        for (char i = 0; i < 48; ++i) {
            if (i == 36) {
                x += 10;
                if (x > MAX_X) {
                    x = MIN_X;
                    y += 10;
                }
                continue;
            }

            draw_sprite(x, y, i, ATTR_HUD, 0);
            x += 10;

            if (x > MAX_X) {
                x = MIN_X;
                y += 10;
            }
        }
        oam_cursor_after_keyboard = __sharpie_get_oam() + 1;
    }

    while (true) {
        __sharpie_set_oam(oam_cursor_after_keyboard);
        clear_screen(COLOR_BG);

        print("MACABRE 'AI'", 10, 1);

        current_frame_input = get_input(0);
        if (NOT(cursor_moving)) {
            if (current_frame_input == BTN_RIGHT) {
                selected_char_x += 10;
                cursor_moving = true;
                cursor_move_delay = CURSOR_INITIAL_DELAY;
            } else if (current_frame_input == BTN_LEFT) {
                selected_char_x -= 10;
                cursor_moving = true;
                cursor_move_delay = CURSOR_INITIAL_DELAY;
            } else if (current_frame_input == BTN_UP) {
                selected_char_y -= 10;
                cursor_moving = true;
                cursor_move_delay = CURSOR_INITIAL_DELAY;
            } else if (current_frame_input == BTN_DOWN) {
                selected_char_y += 10;
                cursor_moving = true;
                cursor_move_delay = CURSOR_INITIAL_DELAY;
            }
        } else {
            if (--cursor_move_delay == 0) {
                cursor_moving = false;
            }
        }

        if (selected_char_x >= MAX_X) {
            if (selected_char_y < MAX_Y) {
                selected_char_x = MIN_X;
                selected_char_y += 10;
            }
        } else if (selected_char_x < MIN_X) {
            if (selected_char_y > MIN_Y) {
                selected_char_x = MAX_X - 9;
                selected_char_y -= 10;
            } else if (selected_char_y == MIN_Y) {
                selected_char_x = LAST_CHAR_POS_X;
                selected_char_y = LAST_CHAR_POS_Y;
            }
        }

        if (selected_char_y > MAX_Y) {
            selected_char_y = MAX_Y;
        } else if (selected_char_y <= MIN_Y) {
            selected_char_y = MIN_Y;
        }

        if (selected_char_x > LAST_CHAR_POS_X &&
            selected_char_y >= LAST_CHAR_POS_Y) {
            selected_char_x = MIN_X;
            selected_char_y = MIN_Y;
        }
        draw_cursor(selected_char_x, selected_char_y);
        restart_frame();

        if (current_frame_input == BTN_A) {
            if (append_delay == 0) {
                unsigned char idx =
                    keyboard_pos_to_char_idx(selected_char_x, selected_char_y);
                if (idx != 255) {
                    input_append(idx);
                }
                append_delay = APPEND_INITIAL_DELAY;
            } else {
                --append_delay;
            }
        } else if (current_frame_input == BTN_B) {
            if (backspace_delay == 0) {
                input_backspace();
                backspace_delay = BACKSPACE_INITIAL_DELAY;
            } else {
                --backspace_delay;
            }
        } else if (current_frame_input == BTN_START) {
            // TODO: trigger generation pass
            input_length = 0;
            input_buffer[0] = 0;
        }

        emit_string(input_buffer, input_length);

        yield();
    }

    return 0;
}

void draw_cursor(unsigned char x, unsigned char y) {
    draw_sprite(x, y - 8, SPR_CURSOR_TOP, ATTR_HUD, 0);
    draw_sprite(x + 8, y, SPR_CURSOR_LEFT, ATTR_HUD, 0);

    draw_sprite(x - 8, y, SPR_CURSOR_RIGHT, ATTR_HFLIP | ATTR_HUD, 0);
    draw_sprite(x, y + 8, SPR_CURSOR_BOTTOM, ATTR_VFLIP | ATTR_HUD, 0);
}

unsigned char keyboard_pos_to_char_idx(unsigned char x, unsigned char y) {
    unsigned char col = (x - MIN_X) / 10;
    unsigned char row = (y - MIN_Y) / 10;

    if (row == 0) {
        if (col > 18)
            return 255;
        return col;
    }

    if (row == 1) {
        if (col > 18)
            return 255;
        if (col <= 16)
            return 19 + col;
        if (col == 17)
            return 36; // space
        return 37;     // .
    }

    if (row == 2) {
        if (col <= 9)
            return 38 + col;
        if (col == 10)
            return 54;
        return 255;
    }

    return 255;
}

void input_append(unsigned char char_idx) {
    // accounting for the last element being the null terminator
    if (input_length >= MAX_INPUT_LENGTH - 1)
        return;
    input_buffer[input_length++] = sprite_to_ascii(char_idx);
    input_buffer[input_length] = 0;
}

void input_backspace(void) {
    if (input_length == 0)
        return;
    input_buffer[--input_length] = 0;
}
