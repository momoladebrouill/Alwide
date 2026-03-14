#include "tools.h"

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

#include "constants.h"

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
  char command[15 + strlen(prog)];
  sprintf(command, "whereis %s", prog);

  FILE* f = popen(command, "r");
  if (f == NULL) {
    return NULL;
  }

  char* path = malloc(PATH_MAX);
  char tmp_shit[PATH_MAX];
  int scan_res = fscanf(f, " %s %s ", tmp_shit, path);
  if (scan_res != 2) {
    free(path);
    return NULL;
  }
  fclose(f);

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

bool getLanguageStringIDForFile(char* lang_id, IO_FileID io_file) {
  /// ---- FILE NAME ----

  if (io_file.status == NONE) {
    return false;
  }

  // Make
  if (strcmp(basename(io_file.path_abs), "Makefile") == 0) {
    strcpy(lang_id, "make");
    return true;
  }

  // Bash
  if (strcmp(basename(io_file.path_abs), "config") == 0 || strcmp(basename(io_file.path_abs), ".bashrc") == 0) {
    strcpy(lang_id, "bash");
    return true;
  }

  /// ---- FILE EXTENSION ----

  // extracting extension
  char* dot = strrchr(io_file.path_args, '.');
  if (dot != NULL)
    strncpy(lang_id, dot + 1, 99);

  // ADD_NEW_LANGUAGE

  // c
  if (strcmp(lang_id, "h") == 0 || strcmp(lang_id, "c") == 0) {
    strcpy(lang_id, "c");
    return true;
  }
  // python
  if (strcmp(lang_id, "py") == 0) {
    strcpy(lang_id, "python");
    return true;
  }
  // markdown
  if (strcmp(lang_id, "md") == 0) {
    strcpy(lang_id, "markdown");
    return true;
  }
  // markdown_inline
  if (strcmp(lang_id, "markdown_inline") == 0) {
    strcpy(lang_id, "markdown_inline");
    return true;
  }
  // java
  if (strcmp(lang_id, "java") == 0) {
    strcpy(lang_id, "java");
    return true;
  }
  // c++
  if (strcmp(lang_id, "cpp") == 0 || strcmp(lang_id, "cc") == 0) {
    strcpy(lang_id, "cpp");
    return true;
  }
  // c#
  if (strcmp(lang_id, "cs") == 0) {
    strcpy(lang_id, "c-sharp");
    return true;
  }
  // css / scss
  if (strcmp(lang_id, "css") == 0 || strcmp(lang_id, "scss") == 0) {
    strcpy(lang_id, "css");
    return true;
  }
  // dart
  if (strcmp(lang_id, "dart") == 0) {
    strcpy(lang_id, "dart");
    return true;
  }
  // go-lang
  if (strcmp(lang_id, "go") == 0) {
    strcpy(lang_id, "go");
    return true;
  }
  // java-script
  if (strcmp(lang_id, "js") == 0) {
    strcpy(lang_id, "javascript");
    return true;
  }
  // json
  if (strcmp(lang_id, "json") == 0) {
    strcpy(lang_id, "json");
    return true;
  }
  // bash/shell
  if (strcmp(lang_id, "sh") == 0 || strcmp(lang_id, "conf") == 0) {
    strcpy(lang_id, "bash");
    return true;
  }
  // scheme implementation
  if (strcmp(lang_id, "scm") == 0) {
    strcpy(lang_id, "query");
    return true;
  }
  // vhdl
  if (strcmp(lang_id, "vhd") == 0) {
    strcpy(lang_id, "vhdl");
    return true;
  }
  // lua
  if (strcmp(lang_id, "lua") == 0) {
    strcpy(lang_id, "lua");
    return true;
  }
  // asm
  if (strcmp(lang_id, "s") == 0) {
    strcpy(lang_id, "asm");
    return true;
  }
  // html
  if (strcmp(lang_id, "html") == 0) {
    strcpy(lang_id, "html");
    return true;
  }

  return false;
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

void countStringFrame(char* ch, int length, int* current_row, int* current_column, int* screen_max_width) {
  assert(current_row != NULL);
  assert(current_column != NULL);

  const int line_length = (screen_max_width == NULL || *screen_max_width == 0) ? INT_MAX : *screen_max_width;

  int current_ch_index = 0;
  int current_line_length = 0;
  int max_line = 0;
  while (current_ch_index < length) {
    if (TAB_CHAR_USE == false) {
      assert(ch[current_ch_index] != '\t');
    }
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
      if (current_line_length + charPrintSize(tmp_ch) > line_length) {
        if (current_line_length > max_line) {
          max_line = current_line_length;
        }
        current_line_length = 0;
        (*current_row)++;
      }
      current_line_length += charPrintSize(tmp_ch);
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


CursorDescriptor positionToCursorDescriptor(Position position) {
  return (CursorDescriptor){.row = position.row + 1, .column = position.column};
}
