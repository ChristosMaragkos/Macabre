#ifndef MACABRE_TABLE_H
#define MACABRE_TABLE_H

#pragma once
#pragma bank 0

#define MACABRE_CHARSET_SIZE 64
#define MACABRE_REAL_CHARS 49
#define MACABRE_VOCAB_SIZE 128
#define MACABRE_SPACE_IDX 36
#define MACABRE_NEWLINE_IDX 48

unsigned char sprite_to_ascii(unsigned int index);
unsigned char ascii_to_sprite(unsigned char sprite_id);

#endif
