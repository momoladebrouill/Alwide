#include "utf_8_extractor.h"

#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

#include "encoding/utf8.h"

void printChar_U8(FILE* f, Char_U8 ch) {
  int size = utf8_size(ch);
  for (int i = 0; i < size; i++) {
    fprintf(f, "%c", ch.t[i]);
  }
}

int sizeChar_U8(Char_U8 ch) { return utf8_size(ch); }

Char_U8 readChar_U8FromFile(FILE* f) {
  Char_U8 ch;
  if (fscanf(f, "%c", ch.t) != 1) {
    return (Char_U8){{0}};
  }
  int size = utf8_size(ch);

  for (char i = 1; i < size; i++) {
    if (fscanf(f, "%c", ch.t + i) != 1) {
      break;
    }
  }

  for (char i = size; i < 4; i++) {
    ch.t[i] = 0;
  }
  return ch;
}

Char_U8 readChar_U8FromFileWithFirst(FILE* f, char c) {
  Char_U8 ch;
  ch.t[0] = c;
  int size = utf8_size(ch);

  for (char i = 1; i < size; i++) {
    if (fscanf(f, "%c", ch.t + i) != 1) {
      break;
    }
  }

  for (char i = size; i < 4; i++) {
    ch.t[i] = 0;
  }
  return ch;
}

Char_U8 readChar_U8FromFileWithFirstUsingFd(int fd, char c) {
  Char_U8 ch;
  ch.t[0] = c;
  int size = utf8_size(ch);

  for (char i = 1; i < size; i++) {
    if (read(fd, ch.t + i, 1) != 1) {
      break;
    }
  }

  for (char i = size; i < 4; i++) {
    ch.t[i] = 0;
  }
  return ch;
}

Char_U8 readChar_U8FromInput(int codepoint) { return unicode_to_utf8((Unicode)codepoint); }

Char_U8 readChar_U8FromCharArray(char* array) {
  Char_U8 ch;
  ch.t[0] = array[0];
  int size = utf8_size(ch);

  for (char i = 1; i < size; i++) {
    ch.t[i] = array[i];
  }

  for (char i = size; i < 4; i++) {
    ch.t[i] = 0;
  }
  return ch;
}

Char_U8 readChar_U8FromCharArrayWithFirst(char* array, char c) {
  Char_U8 ch;
  ch.t[0] = c;
  int size = utf8_size(ch);

  for (char i = 1; i < size; i++) {
    ch.t[i] = array[i];
  }

  for (char i = size; i < 4; i++) {
    ch.t[i] = 0;
  }
  return ch;
}

void printCharToBits(FILE* f, char ch) {
  for (int i = 7; i >= 0; i--) {
    char result = (ch >> i) & 0b1;
    fprintf(f, "%d", result);
  }
  fprintf(f, " ");
}

void printChar_U8ToBits(FILE* f, Char_U8 ch) {
  printCharToBits(f, ch.t[0]);
  printCharToBits(f, ch.t[1]);
  printCharToBits(f, ch.t[2]);
  printCharToBits(f, ch.t[3]);
}

int charPrintSize(Char_U8 ch, int tab_size) {
  if (ch.t[0] == '\t') {
    return tab_size;
  }

  if (utf8_size(ch) == 1) {
    return 1;
  }

  wchar_t wc;
  if (mbtowc(&wc, ch.t, 4) <= 0) {
    return 1;
  }
  int width = wcwidth(wc);
  return (width >= 0) ? width : 1;
}

bool isBetween(Char_U8 ch, char begin, char end) { return ch.t[0] >= begin && ch.t[0] <= end; }

bool isAWordLetter(Char_U8 ch) {
  if (isBetween(ch, 'a', 'z') || isBetween(ch, 'A', 'Z') || isBetween(ch, '0', '9')) {
    return true;
  }

  // High-level letter checks (can be improved by using Unicode properties)
  static const char* word_letters[] = {"_", "ç", "é", "è", "à", "û", "ê", NULL};
  for (int i = 0; word_letters[i] != NULL; i++) {
    if (utf8_equals(ch, readChar_U8FromCharArray((char*)word_letters[i]))) {
      return true;
    }
  }
  return false;
}

bool isInvisible(Char_U8 ch) { return ch.t[0] == ' ' || ch.t[0] == '\t'; }

bool areChar_U8Equals(Char_U8 ch1, Char_U8 ch2) { return utf8_equals(ch1, ch2); }
