#include "utf8.h"

Char_U8 unicode_to_utf8(Unicode c) {
  Char_U8 ch = {{0, 0, 0, 0}};

  if (c <= 0x7F) {
    ch.t[0] = (char)c;
  }
  else if (c <= 0x7FF) {
    ch.t[0] = (char)(0xC0 | (c >> 6));
    ch.t[1] = (char)(0x80 | (c & 0x3F));
  }
  else if (c <= 0xFFFF) {
    ch.t[0] = (char)(0xE0 | (c >> 12));
    ch.t[1] = (char)(0x80 | ((c >> 6) & 0x3F));
    ch.t[2] = (char)(0x80 | (c & 0x3F));
  }
  else if (c <= 0x10FFFF) {
    ch.t[0] = (char)(0xF0 | (c >> 18));
    ch.t[1] = (char)(0x80 | ((c >> 12) & 0x3F));
    ch.t[2] = (char)(0x80 | ((c >> 6) & 0x3F));
    ch.t[3] = (char)(0x80 | (c & 0x3F));
  }
  return ch;
}

Unicode utf8_to_unicode(Char_U8 ch) {
  int size = utf8_size(ch);
  unsigned char* t = (unsigned char*)ch.t;
  Unicode codepoint = 0;

  if (size == 1) {
    codepoint = t[0];
  }
  else if (size == 2) {
    codepoint = ((t[0] & 0x1F) << 6) | (t[1] & 0x3F);
  }
  else if (size == 3) {
    codepoint = ((t[0] & 0x0F) << 12) | ((t[1] & 0x3F) << 6) | (t[2] & 0x3F);
  }
  else if (size == 4) {
    codepoint = ((t[0] & 0x07) << 18) | ((t[1] & 0x3F) << 12) | ((t[2] & 0x3F) << 6) | (t[3] & 0x3F);
  }
  return codepoint;
}

int utf8_size(Char_U8 ch) {
  if ((ch.t[0] >> 7 & 0b1) == 0) {
    return 1;
  }
  if ((ch.t[0] >> 5 & 0b1) == 0) {
    return 2;
  }
  if ((ch.t[0] >> 4 & 0b1) == 0) {
    return 3;
  }
  return 4;
}

bool utf8_equals(Char_U8 ch1, Char_U8 ch2) {
  int size1 = utf8_size(ch1);
  int size2 = utf8_size(ch2);
  if (size1 != size2) {
    return false;
  }

  for (int i = 0; i < size1; i++) {
    if (ch1.t[i] != ch2.t[i]) {
      return false;
    }
  }

  return true;
}
