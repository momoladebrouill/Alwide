#ifndef CONTANTS_H
#define CONTANTS_H

#define LANG_ID_LENGTH 100

// #define PARSE_PRINT

#define SCROLL_SPEED 3
#define NO_EVENT_MOUSE 268435456

#define FILE_NAME_SEPARATOR " | "
#define OPENED_FILE_WINDOW_HEIGHT 2 // 2 to enable 0 to disable

// use make -B after changing the TAB_SIZE
#define TAB_CHAR_USE true
#define TAB_SIZE 2

#define FILE_EXPLORER_WIDTH 30
#define FILE_EXPLORER_TREE_OFFSET 1

#define COLOR_HOVER 8

#define COLOR_HOVER_OFFSET 1000
#define DEFAULT_COLOR_PAIR 1
#define DEFAULT_COLOR_HOVER_PAIR 1001
#define ERROR_COLOR_PAIR 2
#define ERROR_COLOR_HOVER_PAIR 1002

#define MAX_SIZE_FILE_LOGIC 4000000 /*4 Mb*/

extern int color_index;
extern int color_pair;

#endif // CONTANTS_H
