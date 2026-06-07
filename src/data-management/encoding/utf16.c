#include "utf16.h"
#include <stddef.h>
#include "../../data-management/utf_8_extractor.h" // For reading chars from string if needed, or I can use utf8.h functions

int utf16_length(Char_U8 ch) {
  Unicode codepoint = utf8_to_unicode(ch);
  return (codepoint > 0xFFFF) ? 2 : 1;
}

int utf16_string_length(const char* s, int byte_len) {
  if (s == NULL) {
    return 0;
  }
  int index = 0;
  int utf16_len = 0;
  while (index < byte_len && s[index] != '\0') {
    Char_U8 u8 = readChar_U8FromCharArrayWithFirst((char*)s + index, s[index]);
    index += utf8_size(u8);
    utf16_len += utf16_length(u8);
  }
  return utf16_len;
}

int utf16_get_offset(Char_U8* ch, int total_elements, int char_column) {
  int utf16_offset = 0;
  for (int i = 0; i < char_column && i < total_elements; i++) {
    utf16_offset += utf16_length(ch[i]);
  }
  return utf16_offset;
}

int utf16_get_char_column(Char_U8* ch, int total_elements, int utf16_offset) {
  int current_utf16 = 0;
  int i = 0;
  for (; i < total_elements && current_utf16 < utf16_offset; i++) {
    current_utf16 += utf16_length(ch[i]);
  }
  return i;
}
