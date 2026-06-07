#ifndef UTF8_H
#define UTF8_H

#include <stdbool.h>
#include "unicode.h"

/**
 * @brief Represents a single UTF-8 encoded character (up to 4 bytes).
 */
typedef struct {
  char t[4];
} Char_U8;

/**
 * @brief Converts a Unicode codepoint to a Char_U8.
 */
Char_U8 unicode_to_utf8(Unicode c);

/**
 * @brief Converts a Char_U8 to a Unicode codepoint.
 */
Unicode utf8_to_unicode(Char_U8 ch);

/**
 * @brief Returns the number of bytes used by the UTF-8 character (1 to 4).
 */
int utf8_size(Char_U8 ch);

/**
 * @brief Compares two Char_U8 for equality.
 */
bool utf8_equals(Char_U8 ch1, Char_U8 ch2);

#endif // UTF8_H
