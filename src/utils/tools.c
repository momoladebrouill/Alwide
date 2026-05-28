#include "tools.h"
#include "../config/language_feature.h"

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "../environnement/constants.h"
#include "key_management.h"

bool areStringEquals(String str1, String str2) { return strcmp(str1.content, str2.content) == 0; }


time_val timeInMilliseconds(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (((time_val)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

time_val diff2Time(time_val start, time_val end) {
  time_val diff = start - end;
  if (diff < 0) {
    return -diff;
  }
  return diff;
}


int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

int max(int a, int b) {
  if (a > b) {
    return a;
  }
  return b;
}

int numberOfDigitOfNumber(int n) {
  char page_number[40];
  sprintf(page_number, "%d", n);
  return strlen(page_number);
}

int powInt(int x, int y) {
  int res = x;

  for (int i = 1; i < y; i++) {
    res *= x;
  }

  return res;
}

/**
 * Give as fileName the absolute path of the file !
 */
unsigned long long hashFileName(char* fileName) {
  int length = strlen(fileName);
  unsigned long long value = powInt(length, 4);

  for (int i = 0; i < length; i++) {
    value += i * i * i * i * fileName[i];
  }

  return value;
}

void printToNcursesNCharFromString(WINDOW* w, char* str, int n) {
  for (int i = 0; i < n && str[i] != '\0'; i++) {
    wprintw(w, "%c", str[i]);
  }
}


char* whereis(char* prog) {
  char command[PATH_MAX + 20];
  snprintf(command, sizeof(command), "whereis \"%s\"", prog);

  FILE* f = popen(command, "r");
  if (f == NULL) {
    return NULL;
  }

  char* path = malloc(PATH_MAX);
  if (path == NULL) {
    pclose(f);
    return NULL;
  }
  char tmp_shit[PATH_MAX];
  char fmt[64];
  // Limit fscanf to prevent buffer overflow, respecting PATH_MAX
  snprintf(fmt, sizeof(fmt), " %%%ds %%%ds ", PATH_MAX - 1, PATH_MAX - 1);
  int scan_res = fscanf(f, fmt, tmp_shit, path);
  if (scan_res != 2) {
    free(path);
    pclose(f);
    return NULL;
  }
  pclose(f);

  return path;
}


void encodeURI(const char* src, char* dest, size_t dest_size) {
  const char* p = src;
  size_t written = 0;
  while (*p && (written + 1 < dest_size)) {
    if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~' || *p == '/') {
      *dest++ = *p++;
      written++;
    }
    else {
      if (written + 4 > dest_size) {
        break;
      }
      sprintf(dest, "%%%02X", (unsigned char)*p);
      dest += 3;
      written += 3;
      p++;
    }
  }
  *dest = '\0';
}

void getLocalURI(char* realive_abs_path, char* uri) {
  char abs_path[PATH_MAX];
  if (realpath(realive_abs_path, abs_path) == NULL) {
    strncpy(abs_path, realive_abs_path, PATH_MAX - 1);
    abs_path[PATH_MAX - 1] = '\0';
  }
  strcpy(uri, "file://");
  encodeURI(abs_path, uri + 7, URI_MAX - 7);
}

bool isDir(char* path) {
  struct stat file_info;
  stat(path, &file_info);
  return S_ISDIR(file_info.st_mode);
}

// copy from http://www.cse.yorku.ca/~oz/hash.html
int hashString(unsigned char* str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return (int)hash;
}


char* loadFullFile(const char* path, long* length) {
  FILE* f = fopen(path, "rb");
  if (!f) {
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  *length = ftell(f);
  fseek(f, 0, SEEK_SET); /* same as rewind(f); */

  char* string = malloc(*length + 1);
  fread(string, *length, 1, f);
  fclose(f);

  string[*length] = 0;

  return string;
}


// Fonction qui crée récursivement les répertoires comme `mkdir -p`
int mkdir_p(const char* path, mode_t mode) {
  char tmp[1024];
  char* p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = '\0';
  }

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
        perror("mkdir");
        return -1;
      }
      *p = '/';
    }
  }
  if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
    perror("mkdir");
    return -1;
  }
  return 0;
}

void countStringFrame(char* ch, int length, int* current_row, int* current_column, int* screen_max_width,
                      int tab_size) {
  assert(current_row != NULL);
  assert(current_column != NULL);

  const int line_length = (screen_max_width == NULL || *screen_max_width == 0) ? INT_MAX : *screen_max_width;

  int current_ch_index = 0;
  int current_line_length = 0;
  int max_line = 0;
  while (current_ch_index < length) {
    if (ch[current_ch_index] == '\n') {
      (*current_row)++;
      *current_column = 0;
      if (current_line_length > max_line) {
        max_line = current_line_length;
      }
      current_line_length = 0;
    }
    else {
      Char_U8 tmp_ch = readChar_U8FromCharArray(ch + current_ch_index);
      current_ch_index += sizeChar_U8(tmp_ch) - 1;
      (*current_column)++;
      if (current_line_length + charPrintSize(tmp_ch, tab_size) > line_length) {
        if (current_line_length > max_line) {
          max_line = current_line_length;
        }
        current_line_length = 0;
        (*current_row)++;
      }
      current_line_length += charPrintSize(tmp_ch, tab_size);
    }
    current_ch_index++;
  }
  if (current_line_length > max_line) {
    max_line = current_line_length;
  }

  if (screen_max_width != NULL) {
    *screen_max_width = max_line;
  }
}

int countStringUTF16Length(const char* ch, int length) {
  if (ch == NULL) {
    return 0;
  }
  int index = 0;
  int utf16_len = 0;
  while (index < length && ch[index] != '\0') {
    Char_U8 u8 = readChar_U8FromCharArrayWithFirst((char*)ch + index, ch[index]);
    index += sizeChar_U8(u8);
    utf16_len += getUTF16Length(u8);
  }
  return utf16_len;
}

int getUTF16Offset(Char_U8* ch, int element_number, int character_column) {
  int utf16_offset = 0;
  for (int i = 0; i < character_column && i < element_number; i++) {
    utf16_offset += getUTF16Length(ch[i]);
  }
  return utf16_offset;
}

int getCharacterColumnFromUTF16Offset(Char_U8* ch, int element_number, int utf16_offset) {
  int current_utf16 = 0;
  int i = 0;
  for (; i < element_number && current_utf16 < utf16_offset; i++) {
    current_utf16 += getUTF16Length(ch[i]);
  }
  return i;
}

int getByteOffset(Char_U8* ch, int element_number, int character_column) {
  int byte_offset = 0;
  for (int i = 0; i < character_column && i < element_number; i++) {
    byte_offset += sizeChar_U8(ch[i]);
  }
  return byte_offset;
}

char* trim(char* ch) {
  while (*ch != '\0' && isblank(*ch)) {
    ch++;
  }
  return ch;
}

static int hex_to_int(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  return -1;
}

void decodeURI(const char* src, char* dest, size_t dest_size) {
  const char* p = src;
  size_t written = 0;

  if (strncmp(p, "file://", 7) == 0) {
    p += 7;
    // Handle 'file:///path' (empty authority) vs 'file://localhost/path' (with authority)
    if (*p != '/') {
      const char* first_slash = strchr(p, '/');
      if (first_slash) {
        p = first_slash;
      }
    }
  }

  while (*p && (written + 1 < dest_size)) {
    if (*p == '%' && isxdigit(p[1]) && isxdigit(p[2])) {
      int high = hex_to_int(p[1]);
      int low = hex_to_int(p[2]);
      if (high != -1 && low != -1) {
        *dest++ = (char)((high << 4) | low);
        p += 3;
        written++;
        continue;
      }
    }
    *dest++ = *p++;
    written++;
  }
  *dest = '\0';
}


CursorDescriptor positionToCursorDescriptor(LSP_Position position) {
  return (CursorDescriptor){.row = LSP_0_row_to_1_row(position.row), .column = position.column};
}


// --- Conversion Helpers ---

LSP_Position LSP_pos_from_cursor(Cursor cursor) {
  int character_col = cursor.line_id.absolute_column;
  LineNode* line = cursor.line_id.line;
  int utf16_col = getUTF16Offset(line->ch, line->element_number, character_col);
  return (LSP_Position){.row = cursor.file_id.absolute_row - 1, .column = utf16_col};
}

LSP_Range LSP_range_from_cursor(Cursor c1, Cursor c2) {
  return (LSP_Range){.pos1 = LSP_pos_from_cursor(c1), .pos2 = LSP_pos_from_cursor(c2)};
}


int LSP_0_row_to_1_row(int lsp_row) {
  if (lsp_row == INT_MAX) {
    return INT_MAX;
  }
  return lsp_row + 1;
}

Cursor LSP_tryToReachCursorForLSPPosition(Cursor cursor, LSP_Position position) {
  Cursor target_row = tryToReachAbsPosition(cursor, LSP_0_row_to_1_row(position.row), 0);
  int character_col = getCharacterColumnFromUTF16Offset(target_row.line_id.line->ch, target_row.line_id.line->element_number,
                                                        position.column);
  return tryToReachAbsPosition(target_row, LSP_0_row_to_1_row(position.row), character_col);
}

int normalize_legacy(int c) {
  if (c == ERR) {
    return ERR;
  }

  /* 1. Handle Ncurses-translated shifted/modified keys */
  switch (c) {
    case KEY_BTAB:
      return K(K_MOD_SHIFT, KEY_BTAB);
    case KEY_SRIGHT:
      return K(K_MOD_SHIFT, KEY_RIGHT);
    case KEY_SLEFT:
      return K(K_MOD_SHIFT, KEY_LEFT);
    case KEY_SHOME:
      return K(K_MOD_SHIFT, KEY_HOME);
    case KEY_SEND:
      return K(K_MOD_SHIFT, KEY_END);
    case KEY_SDC:
      return K(K_MOD_SHIFT, KEY_DC);
    case KEY_SIC:
      return K(K_MOD_SHIFT, KEY_IC);
    case KEY_SNEXT:
      return K(K_MOD_SHIFT, KEY_NPAGE);

    // Shift + Up/Down (often KEY_SR / KEY_SF)
    case KEY_SR:
      return K(K_MOD_SHIFT, KEY_UP);
    case KEY_SF:
      return K(K_MOD_SHIFT, KEY_DOWN);

    // Hardcoded fallbacks for the logged system keycodes
    case 554:
      return K(K_MOD_CTRL, KEY_LEFT);
    case 569:
      return K(K_MOD_CTRL, KEY_RIGHT);
    case 575:
      return K(K_MOD_CTRL, KEY_UP);
    case 534:
      return K(K_MOD_CTRL, KEY_DOWN);

    case 555:
      return K(K_MOD_CTRL | K_MOD_SHIFT, KEY_LEFT);
    case 570:
      return K(K_MOD_CTRL | K_MOD_SHIFT, KEY_RIGHT);
    case 576:
      return K(K_MOD_CTRL | K_MOD_SHIFT, KEY_UP);
    case 535:
      return K(K_MOD_CTRL | K_MOD_SHIFT, KEY_DOWN);
  }

  /* 2. Map ASCII control codes 1-31 to Unified format. */
  if (((c >= 1 && c <= 31) || c == 0) && c != 9 && c != 10 && c != 13) {
    int base;
    if (c == 0) {
      base = ' ';
    }
    else if (c >= 1 && c <= 26) {
      base = c + 'a' - 1;
    }
    else if (c == 27) {
      return H_KEY_ESCAPE;
    }
    else if (c == 28) {
      base = '\\';
    }
    else if (c == 29) {
      base = ']';
    }
    else if (c == 30) {
      base = '^';
    }
    else if (c == 31) {
      base = '_';
    }
    else {
      base = c;
    }
    return K(K_MOD_CTRL, base);
  }

  /* 3. Standard keycodes */
  switch (c) {
    case 9:
      return H_KEY_TAB;
    case 10:
    case 13:
      return H_KEY_ENTER;
    case 127:
      return K(0, KEY_BACKSPACE);
    default:
      if (c > 255) {
        /* Ultimate fallback: parse dynamically via Ncurses terminfo keyname */
        const char* name = keyname(c);
        if (name && strlen(name) >= 4 && name[0] == 'k') {
          int target_key = 0;
          if (strncmp(name, "kUP", 3) == 0) {
            target_key = KEY_UP;
          }
          else if (strncmp(name, "kDN", 3) == 0) {
            target_key = KEY_DOWN;
          }
          else if (strncmp(name, "kLFT", 4) == 0) {
            target_key = KEY_LEFT;
          }
          else if (strncmp(name, "kRGT", 4) == 0) {
            target_key = KEY_RIGHT;
          }
          else if (strncmp(name, "kHOM", 4) == 0) {
            target_key = KEY_HOME;
          }
          else if (strncmp(name, "kEND", 4) == 0) {
            target_key = KEY_END;
          }
          else if (strncmp(name, "kDC", 3) == 0) {
            target_key = KEY_DC;
          }
          else if (strncmp(name, "kIC", 3) == 0) {
            target_key = KEY_IC;
          }

          if (target_key != 0) {
            int len = strlen(name);
            char last_char = name[len - 1];
            if (isdigit((unsigned char)last_char)) {
              int mod_num = last_char - '0';
              int val = mod_num - 1;
              if (val < 0) {
                val = 0;
              }
              int unified_mods = 0;
              if (val & 1) {
                unified_mods |= K_MOD_SHIFT;
              }
              if (val & 2) {
                unified_mods |= K_MOD_ALT;
              }
              if (val & 4) {
                unified_mods |= K_MOD_CTRL;
              }
              if (val & 8) {
                unified_mods |= K_MOD_SUPER;
              }
              return K(unified_mods, target_key);
            }
          }
        }
        return K(0, c); /* Other Ncurses keys */
      }
      return c; /* Standard printable byte */
  }
}
