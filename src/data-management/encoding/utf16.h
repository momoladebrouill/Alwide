#ifndef UTF16_H
#define UTF16_H

#include "utf8.h"

/**
 * @brief Returns the length in UTF-16 code units for a given Char_U8.
 * (1 for BMP, 2 for astral planes/surrogate pairs).
 */
int utf16_length(Char_U8 ch);

/**
 * @brief Returns the total UTF-16 length of a UTF-8 string.
 */
int utf16_string_length(const char* s, int byte_len);

/**
 * @brief Calculates the UTF-16 offset (in code units) corresponding to a character column.
 */
int utf16_get_offset(Char_U8* ch, int total_elements, int char_column);

/**
 * @brief Calculates the character column corresponding to a UTF-16 offset.
 */
int utf16_get_char_column(Char_U8* ch, int total_elements, int utf16_offset);

#endif // UTF16_H
