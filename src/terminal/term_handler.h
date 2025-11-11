#ifndef NCURSES_HANDLER_H
#define NCURSES_HANDLER_H
#include <ncurses.h>
#include <wchar.h>

#include "../data-management/file_management.h"
#include "../data-management/file_structure.h"
#include "../io_management/io_explorer.h"
#include "highlight.h"
#include "windows/gui_entities.h"

/* Unix call, use 'man wcwidth' to see explication. */
int wcwidth(const wint_t wc);


////// -------------- WINDOWS MANAGEMENTS --------------

void initGUIContext(GUIContext* gui_context);

void initNCurses(GUIContext* gui_context);

void setFocus(GUIContext* gui_context, WINDOW* w);

void resetFocus(GUIContext* gui_context);

////// -------------- PRINT FUNCTIONS --------------

void repaintGUI(GUIContext* gui_context, WindowHighlightDescriptor* highlight_descriptor, ExplorerFolder* explorer,
                FileContainer* files, int file_count, int current_file);

void printChar_U8ToNcurses(WINDOW* w, Char_U8 ch);

LineMarker getMarkerForCurrentLine(int row, WindowHighlightDescriptor* highlight_descriptor, int whd_offset,
                                   Diagnostic** diagnostic);

void updateEDW(GUIContext* gui_context);

void updateFEW(GUIContext* gui_context);

void updateOFW(GUIContext* gui_context);

void updateGUI(GUIContext* gui_context);

////// -------------- UTILS FUNCTIONS --------------

void moveScreenToMatchCursor(GUIContext* context, Cursor cursor, int* screen_x, int* screen_y);

void centerCursorOnScreen(GUIContext* context, Cursor cursor, int* screen_x, int* screen_y);

int getScreenXForCursor(Cursor cursor, int screen_x);

LineIdentifier getLineIdForScreenX(LineIdentifier line_id, int screen_x, int x_click);

void setDesiredColumn(Cursor cursor, int* desired_column);

void printToWindow(WINDOW*w, char *ch, int length, int offset_x, int offset_y, int line_length);




#endif // NCURSES_HANDLER_H
