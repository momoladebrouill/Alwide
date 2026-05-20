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

bool areStringEquals(String str1, String str2) { return strcmp(str1.content, str2.content) == 0; }


time_val timeInMilliseconds(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (((time_val)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

time_val diff2Time(time_val start, time_val end) {
  time_val diff = start - end;
  if (diff < 0)
    return -diff;
  return diff;
}


int min(int a, int b) {
  if (a < b)
    return a;
  return b;
}

int max(int a, int b) {
  if (a > b)
    return a;
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
      if (written + 4 > dest_size)
        break;
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

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

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
  if (tmp[len - 1] == '/')
    tmp[len - 1] = '\0';

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

void countStringFrame(char* ch, int length, int* current_row, int* current_column, int* screen_max_width, int tab_size) {
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

char* trim(char* ch) {
  while (*ch != '\0' && isblank(*ch)) {
    ch++;
  }
  return ch;
}

static int hex_to_int(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
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
      if (first_slash)
        p = first_slash;
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

LSP_Position LSP_pos_from_cursor(int ww_row, int ww_col) { return (LSP_Position){.row = ww_row - 1, .column = ww_col}; }

LSP_Range LSP_range_from_cursor(int r1, int c1, int r2, int c2) {
  return (LSP_Range){.pos1 = LSP_pos_from_cursor(r1, c1), .pos2 = LSP_pos_from_cursor(r2, c2)};
}


int LSP_0_row_to_1_row(int lsp_row) { return lsp_row + 1; }

Cursor LSP_tryToReachCursorForLSPPosition(Cursor cursor, LSP_Position position) {
  return tryToReachAbsPosition(cursor, LSP_0_row_to_1_row(position.row), position.column);
}
