#include "font.h"
#include "include/sharpie.h"

#define true 1
#define false 0
#define NOT(b) ((b) == false)

const unsigned char MIN_X = 32;
const unsigned char MAX_X = 221;

const unsigned char MIN_Y = 210;
const unsigned char MAX_Y = 230;

const unsigned char COLOR_BG = 14;

const unsigned char LAST_CHAR_POS_X = 112;
const unsigned char LAST_CHAR_POS_Y = 230;

// we reuse the same sprites, just flipped. Retro console dev, baby!
const unsigned char SPR_CURSOR_RIGHT = FONT_SPRITE_COUNT - 3;
const unsigned char SPR_CURSOR_LEFT = FONT_SPRITE_COUNT - 3;

const unsigned char SPR_CURSOR_TOP = FONT_SPRITE_COUNT - 4;
const unsigned char SPR_CURSOR_BOTTOM = FONT_SPRITE_COUNT - 4;

// How many frames can the cursor not be moved for,
// once it's moved once?
const unsigned char CURSOR_INITIAL_DELAY = 8;

unsigned char selected_char_x = MIN_X;
unsigned char selected_char_y = MIN_Y;

unsigned int oam_cursor_after_keyboard = 0;

Button current_frame_input = BTN_NONE;

unsigned char cursor_moving = false;
unsigned char cursor_move_delay = 0;

void draw_cursor(unsigned char x, unsigned char y);

int main(void) {
    asm("ATTR 17");

    clear_screen(COLOR_BG);
    {
        unsigned char x = MIN_X;
        unsigned char y = MIN_Y;

        for (char i = 0; i < 48; ++i) {
            if (i == 36) {
                continue;
            }

            draw_sprite(x, y, i, 0, 0);
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
        yield();
    }

    return 0;
}

void draw_cursor(unsigned char x, unsigned char y) {
    draw_sprite(x, y - 8, SPR_CURSOR_TOP, ATTR_NONE, 0);
    draw_sprite(x + 8, y, SPR_CURSOR_LEFT, ATTR_NONE, 0);

    draw_sprite(x - 8, y, SPR_CURSOR_RIGHT, ATTR_HFLIP, 0);
    draw_sprite(x, y + 8, SPR_CURSOR_BOTTOM, ATTR_VFLIP, 0);
}
