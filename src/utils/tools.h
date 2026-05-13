#ifndef TOOLS_H
#define TOOLS_H

#include <linux/limits.h>
#include <ncurses.h>
#include <stdint.h>
#include <sys/types.h>

#include "../advanced/lsp/lsp_client.h"
#include "../io-management/io_manager.h"

#define URI_MAX (PATH_MAX * 3 + 8)

typedef struct {
  const char* content;
  uint32_t length;
} String;

bool areStringEquals(String str1, String str2);


typedef long long time_val;

time_val timeInMilliseconds(void);

time_val diff2Time(time_val start, time_val end);

int min(int a, int b);

int max(int a, int b);

int numberOfDigitOfNumber(int n);

unsigned long long hashFileName(char* fileName);

void printToNcursesNCharFromString(WINDOW* w, char* str, int n);

char* whereis(char* prog);

void getLocalURI(char* realive_abs_path, char* uri);

bool isDir(char* path);

bool getLanguageStringIDForFile(char* lang, IO_FileID io_file);

int hashString(unsigned char* str);

char* loadFullFile(const char* path, long* length);

int mkdir_p(const char* path, mode_t mode);

void countStringFrame(char* ch, int length, int* current_row, int* current_column, int* screen_max_width);

char* trim(char* ch);

void decodeURI(const char* src, char* dest, size_t dest_size);

void encodeURI(const char* src, char* dest, size_t dest_size);

CursorDescriptor positionToCursorDescriptor(LSP_Position position);

// Strictly 0-based LSP position constructor
// Convert from WishWim (1-based row, 0-based col) to LSP (0-based row, 0-based col)
LSP_Position LSP_pos_from_cursor(int ww_row, int ww_col);
LSP_Range LSP_range_from_cursor(int r1, int c1, int r2, int c2);
Cursor LSP_tryToReachCursorForLSPPosition(Cursor cursor, LSP_Position position);
int LSP_0_row_to_1_row(int lsp_row);


#endif // TOOLS_H
