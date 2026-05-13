#ifndef NCURSES_HANDLER_H
#define NCURSES_HANDLER_H
#include <ncurses.h>
#include <wchar.h>

#include "../data-management/file_management.h"
#include "../data-management/file_structure.h"
#include "../io-management/io_explorer.h"
#include "highlight.h"
#include "windows/gui_entities.h"


////// -------------- WINDOWS MANAGEMENTS --------------

void gui_initGUIContext(GUIContext* gui_context);

void gui_initNCurses(GUIContext* gui_context);

void gui_setFocus(GUIContext* gui_context, WINDOW* w);

void gui_resetFocus(GUIContext* gui_context);

////// -------------- PRINT FUNCTIONS --------------

void gui_repaintGUI(GUIContext* gui_context, WindowHighlightDescriptor* highlight_descriptor, ExplorerFolder* explorer,
                FileContainer* files, int file_count, int current_file);

void gui_printChar_U8ToNcurses(WINDOW* w, Char_U8 ch);

LineMarker gui_getMarkerForCurrentLine(int row, WindowHighlightDescriptor* highlight_descriptor, int whd_offset,
                                   void** diagnostic);

void gui_updateEDW(GUIContext* gui_context);

void gui_updateFEW(GUIContext* gui_context);

void gui_updateOFW(GUIContext* gui_context);

void gui_updateGUI(GUIContext* gui_context);

bool gui_doesGUINeedRepaint(GUIContext* gui_context);

////// -------------- UTILS FUNCTIONS --------------

void moveScreenToMatchCursor(GUIContext* context, Cursor cursor, int* screen_x, int* screen_y);

void centerCursorOnScreen(GUIContext* context, Cursor cursor, int* screen_x, int* screen_y);

int getScreenXForCursor(Cursor cursor, int screen_x);

LineIdentifier getLineIdForScreenX(LineIdentifier line_id, int screen_x, int x_click);

void setDesiredColumn(Cursor cursor, int* desired_column);

void printToWindow(WINDOW* w, char* ch, int length, int offset_x, int offset_y, int line_length, int max_line_number);


#endif // NCURSES_HANDLER_H
