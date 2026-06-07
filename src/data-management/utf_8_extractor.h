#ifndef UTF_8_EXTRACTOR_H
#define UTF_8_EXTRACTOR_H

#include <stdbool.h>
#include <stdio.h>

#include "encoding/utf8.h"

/**
 * @file utf_8_extractor.h
 * @brief High-level UTF-8 I/O and properties.
 */

/**
 * Print the Char_U8 to the file.
 */
void printChar_U8(FILE* f, Char_U8 ch);

/**
 * Return on how many bytes is coded this Char_U8.
 * @deprecated Use utf8_size(ch) from utf8.h
 */
int sizeChar_U8(Char_U8 ch);

/**
 * Return the first Char_U8 of the current file.
 */
Char_U8 readChar_U8FromFile(FILE* f);

/**
 * Return the first Char_U8 from FILE with first c.
 */
Char_U8 readChar_U8FromFileWithFirst(FILE* f, char c);

/**
 * Return the first Char_U8 from FILE with first c using fd.
 */
Char_U8 readChar_U8FromFileWithFirstUsingFd(int fd, char c);

/**
 * Return the first Char_U8 from a Unicode codepoint.
 * @deprecated Use unicode_to_utf8(c) from utf8.h
 */
Char_U8 readChar_U8FromInput(int c);

/**
 * Return the first Char_U8 of char*.
 */
Char_U8 readChar_U8FromCharArray(char* ch);

/**
 * Return the first Char_U8 of char* with first c.
 */
Char_U8 readChar_U8FromCharArrayWithFirst(char* array, char c);

/**
 * Print in the file the bits of a char.
 */
void printCharToBits(FILE* f, char ch);

/**
 * Print in the file the bits of a Char_U8.
 */
void printChar_U8ToBits(FILE* f, Char_U8 ch);

/**
 * Returns the visual width of the character (e.g., 2 for wide characters, tab_size for \t).
 */
int charPrintSize(Char_U8 ch, int tab_size);

/**
 * Checks if the first byte of the character is between begin and end.
 */
bool isBetween(Char_U8 ch, char begin, char end);

/**
 * Checks if the character is a word-constituent letter.
 */
bool isAWordLetter(Char_U8 ch);

/**
 * Checks if the character is a space or tab.
 */
bool isInvisible(Char_U8 ch);

/**
 * Compares two Char_U8 for equality.
 * @deprecated Use utf8_equals(ch1, ch2) from utf8.h
 */
bool areChar_U8Equals(Char_U8 ch1, Char_U8 ch2);

/**
 * Return the UTF-16 length of the character.
 * @deprecated Use utf16_length(ch) from utf16.h
 */
int getUTF16Length(Char_U8 ch);

#endif // UTF_8_EXTRACTOR_H
