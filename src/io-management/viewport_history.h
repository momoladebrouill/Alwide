#ifndef FILE_HISTORY_H
#define FILE_HISTORY_H

#include "../data-management/file_management.h"
#include "../terminal/windows/gui_entities.h"
#include "io_manager.h"


#define FILE_HISTORY_PATH "/.config/al/.file_history/"

typedef struct {
  gui_Context* gui;
  int* screen_x;
  int* screen_y;
} ViewPort;


void getLastFilePosition(char* fileName, int* row, int* column, int* screen_x, int* screen_y);

void setlastFilePosition(char* fileName, int row, int column, int screen_x, int screen_y);

void fetchSavedCursorPosition(IO_FileID file, Cursor* cursor, int* screen_x, int* screen_y);

ViewPort viewPortOf(gui_Context* context, int* screen_x, int* screen_y);

#endif // FILE_HISTORY_H
