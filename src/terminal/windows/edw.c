#include "edw.h"

#include <assert.h>
#include <string.h>

#include "../../data-management/file_management.h"
#include "../highlight.h"
#include "../term_handler.h"
#include "gui_entities.h"
#include "pow.h"

void gui_initEDWContext(EDW_GUIContext* context) {
  context->ftw = NULL; // File Text Window
  context->lnw = NULL; // Line Number Window
  context->pow = NULL; // Popup Window

  context->refresh_edw = true; // Need to reprint editor window
  context->length_line_number = 0;

  // popup init values
  context->show_pow = false;
  context->pow_owner = NO_OWNER;
  context->completion_offset_y = 0;
  context->completion_selected = 0;
  context->lastTextAnchor.row = 0;
  context->lastTextAnchor.column = 0;
}


void gui_resizeEDW(GUIContext* gui_context, int length_line_number) {

  if (length_line_number != -1) {
    gui_context->edw_context.length_line_number = length_line_number;
  }

  delwin(gui_context->edw_context.ftw);
  delwin(gui_context->edw_context.lnw);
  gui_context->edw_context.ftw =
    newwin(0, 0, gui_context->ofw_context.ofw_height,
           gui_context->edw_context.length_line_number + 1 + gui_context->few_context.few_width);
  gui_context->edw_context.lnw = newwin(0, gui_context->edw_context.length_line_number + 1,
                                        gui_context->ofw_context.ofw_height, gui_context->few_context.few_width);

  wbkgd(gui_context->edw_context.ftw, COLOR_PAIR(DEFAULT_COLOR_PAIR));
  gui_context->edw_context.refresh_edw = true;
}


void printEditor_printLineNumber(EDW_GUIContext* context, Cursor cursor, int screen_y, FileIdentifier file_cur, int row,
                                 WindowHighlightDescriptor* highlight_descriptor, int whd_offset) {
  char line_number[40];
  sprintf(line_number, "%d", file_cur.absolute_row);
  int lineNumberSize = strlen(line_number);

  NCURSES_PAIRS_T color = DEFAULT_COLOR_PAIR;
  attr_t attr = A_BOLD | A_ITALIC;


  LineMarker marker = getMarkerForCurrentLine(row, highlight_descriptor, whd_offset, NULL);
  switch (marker) {
    case LSP_ERROR:
      color = ERROR_COLOR_PAIR;
      break;
    case LSP_WARNING:
      color = WARNING_COLOR_PAIR;
      break;
    case LSP_INFORMATION:
      color = INFO_COLOR_PAIR;
      break;
    default:
      attr = A_DIM;
      break;
  }

  wattr_set(context->lnw, attr, color, NULL);

  if (file_cur.absolute_row == cursor.file_id.absolute_row) {
    attr = A_BOLD;
    wattroff(context->lnw, A_DIM);
    wattron(context->lnw, A_BOLD);
  }
  else {
    attr = A_DIM;
  }

  wmove(context->lnw, row - screen_y, 0);
  for (int i = 0; i < getmaxx(context->lnw) - lineNumberSize - 1; i++) {
    wprintw(context->lnw, " ");
  }
  wprintw(context->lnw, "%s", line_number);
  wattr_set(context->lnw, attr, DEFAULT_COLOR_PAIR, NULL);
  wprintw(context->lnw, "│");
}


void printEditor_printFileContent(EDW_GUIContext* context, Cursor cursor, Cursor select_cursor, int screen_x,
                                  WindowHighlightDescriptor* highlight_descriptor, FileIdentifier file_cur,
                                  const int column_count, int* whd_offset) {
  LineIdentifier begin_screen_line_cur =
    tryToReachAbsColumn(moduloLineIdentifierR(getLineForFileIdentifier(file_cur), 0), screen_x);
  LineIdentifier end_screen_line_cur = tryToReachAbsColumn(begin_screen_line_cur, screen_x + column_count - 3);

  int column = screen_x;
  while (end_screen_line_cur.absolute_column >= screen_x &&
         begin_screen_line_cur.absolute_column <= end_screen_line_cur.absolute_column &&
         screen_x + column_count - 3 >= column) {
    Char_U8 ch = getCharForLineIdentifier(begin_screen_line_cur);
    Cursor ch_cursor = cursorOf(file_cur, begin_screen_line_cur);

    int size = charPrintSize(ch);
    // If the char is detected as not printable char.
    if (size <= 0) {
      ch = readChar_U8FromCharArray("�");
      size = 1;
    }

    // If a char of size 2 is at the end of the line replace it by '_' to avoid line overflow.
    if (size >= 2 && screen_x + column_count - 4 < column) {
      ch = readChar_U8FromCharArray("_");
      // size = 1;
    }

    // determine if the char is selected or not.
    bool selected_style =
      isCursorDisabled(select_cursor) == false && isCursorBetweenOthers(ch_cursor, select_cursor, cursor);

    // get current highlight.
    TextPartHighlightDescriptor* current_highlight =
      whd_tphd_forCursorWithOffsetIndex(highlight_descriptor, ch_cursor, whd_offset);


    // default style
    attr_t attr = A_NORMAL;
    NCURSES_PAIRS_T color = DEFAULT_COLOR_PAIR;

    // override default style if a textPartHighlight was found.
    if (current_highlight != NULL) {
      attr = current_highlight->attributes;
      color = current_highlight->color;
    }

    // add the offset if the current text is selected
    if (selected_style) {
      color += COLOR_HOVER_OFFSET;
    }

    // setting the char attribute.
    wattr_set(context->ftw, attr, color, 0);

    if (ch.t[0] == '\t') {
      Char_U8 space = readChar_U8FromInput(' ');
      for (int i = 0; i < TAB_SIZE; i++) {
        printChar_U8ToNcurses(context->ftw, space);
      }
    }
    else {
      printChar_U8ToNcurses(context->ftw, ch);
    }

    // move to next column
    begin_screen_line_cur.relative_column++;
    begin_screen_line_cur.absolute_column++;
    column += size;
  }

  // show empty line selected.
  if (begin_screen_line_cur.absolute_column == end_screen_line_cur.absolute_column &&
      hasElementAfterLine(end_screen_line_cur) == false) {
    if (isCursorDisabled(select_cursor) == false &&
        isCursorBetweenOthers(cursorOf(file_cur, begin_screen_line_cur), select_cursor, cursor)) {
      // if line selected
      wattr_set(context->ftw, A_NORMAL, DEFAULT_COLOR_HOVER_PAIR, 0);
      wprintw(context->ftw, " ");
      wattr_set(context->ftw, A_NORMAL, 0, NULL);
    }
  }

  // If the line is not fully display show '>'
  if (hasElementAfterLine(end_screen_line_cur)) {
    wattr_set(context->ftw, A_BOLD | A_UNDERLINE | A_DIM, DEFAULT_COLOR_PAIR, 0);
    wprintw(context->ftw, ">");
  }

  wprintw(context->ftw, "\n");
}


void printEditor_printCursor(EDW_GUIContext* context, Cursor cursor, int screen_x, int screen_y,
                             WindowHighlightDescriptor* highlight_descriptor, const int line_count,
                             const int column_count) {
  // Check if cursor is in the screen and print it if needed.
  if (cursor.file_id.absolute_row >= screen_y && cursor.file_id.absolute_row < screen_y + line_count &&
      cursor.line_id.absolute_column >= screen_x - 1 && cursor.line_id.absolute_column <= screen_x + column_count - 3) {
    int x = getScreenXForCursor(cursor, screen_x);
    wmove(context->ftw, cursor.file_id.absolute_row - screen_y, x);

    TextPartHighlightDescriptor* current_highlight = NULL;

    char size = 1;
    if (hasElementAfterLine(cursor.line_id) == true) {
      // Check the size of the the char which is under cursor.
      Cursor tmp = moveRight(cursor);
      size = charPrintSize(getCharForLineIdentifier(tmp.line_id));
      int unused = 0;
      current_highlight = whd_tphd_forCursorWithOffsetIndex(highlight_descriptor, tmp, &unused);
    }
    else {
      int unused = 0;
      current_highlight = whd_tphd_forCursorWithOffsetIndex(highlight_descriptor, cursor, &unused);
    }


    attr_t attr = A_STANDOUT;
    NCURSES_PAIRS_T color = DEFAULT_COLOR_PAIR;

    if (current_highlight != NULL) {
      attr |= current_highlight->attributes;
      color = current_highlight->color;
    }

    wchgat(context->ftw, size, attr, color, NULL);
  }
}


void move_physical_cursor(EDW_GUIContext* context, Cursor cursor, int screen_x, int screen_y, const int line_count,
                          const int column_count) {
  int ftw_x = getScreenXForCursor(cursor, screen_x);
  if (cursor.file_id.absolute_row >= screen_y && cursor.file_id.absolute_row < screen_y + line_count && ftw_x >= 0 &&
      ftw_x <= getmaxx(context->ftw) - 3) {
    curs_set(1);
    int abs_y = cursor.file_id.absolute_row - screen_y + getbegy(context->ftw);
    int abs_x = ftw_x + getbegx(context->ftw);
    move(abs_y, abs_x);
  }
  else {
    curs_set(0);
  }
}


void gui_repaintEDW(EDW_GUIContext* context, Cursor cursor, Cursor select_cursor, int screen_x, int screen_y,
                    WindowHighlightDescriptor* highlight_descriptor, LSP_ComputedData* lsp_data) {
  if (!context->refresh_edw) {
    return;
  }
  wmove(context->ftw, 0, 0);
  FileIdentifier file_cur = cursor.file_id;

  const int line_count = getmaxy(context->ftw);
  const int column_count = getmaxx(context->ftw);


  // ===============  FOR EACH LINE  ===============
  int whd_offset = 0;
  for (int row = screen_y; row < screen_y + line_count; row++) {
    // getting the row to print.
    file_cur = tryToReachAbsRow(file_cur, row);

    // if the row is couldn't be reached skip it.
    if (file_cur.absolute_row != row) {
      // show empty line number.
      wmove(context->lnw, row - screen_y, 0);
      for (int i = 0; i < getmaxx(context->lnw); i++) {
        wprintw(context->lnw, " ");
      }

      // show ~ as content in file text window
      wattr_set(context->ftw, A_NORMAL, DEFAULT_COLOR_PAIR, NULL);
      wprintw(context->ftw, "~\n");

      continue;
    }

    // ===============  Print line number  ===============
    printEditor_printLineNumber(context, cursor, screen_y, file_cur, row, highlight_descriptor, whd_offset);


    // ===============  Print File Content  ===============

    printEditor_printFileContent(context, cursor, select_cursor, screen_x, highlight_descriptor, file_cur, column_count,
                                 &whd_offset);
  }

  // ===============  Print Cursor  ===============
#ifdef SIMULATED_CURSOR
  printEditor_printCursor(context, cursor, screen_x, screen_y, highlight_descriptor, line_count, column_count);
#else
  move_physical_cursor(context, cursor, screen_x, screen_y, line_count, column_count);
#endif

  wnoutrefresh(context->lnw);
  wnoutrefresh(context->ftw);

  if (context->show_pow) {
    assert(context->pow != NULL);
    gui_printPopup(context, &cursor, lsp_data);
    wnoutrefresh(context->pow);
  }

  context->refresh_edw = false;
}

int getEDW_LengthLineNumber(GUIContext* gui_context) { return gui_context->edw_context.length_line_number; }

bool gui_showPopup(GUIContext* gui_context, int y, int x, int height, int width, PopupOwner owner) {
  delwin(gui_context->edw_context.pow);
  gui_context->edw_context.pow = newwin(height, width, y - height + 1 + getbegy(gui_context->edw_context.ftw),
                                        x + getbegx(gui_context->edw_context.ftw) + 2);

  gui_context->edw_context.show_pow = gui_context->edw_context.pow != NULL;
  if (gui_context->edw_context.show_pow) {
    gui_context->edw_context.pow_owner = owner;
  }
  gui_context->edw_context.refresh_edw = true;

  return gui_context->edw_context.show_pow;
}

bool gui_adaptPopup(GUIContext* gui_context, int slice_x, int slice_y) {
  if (gui_context->edw_context.pow == NULL) {
    return false;
  }

  switch (gui_context->edw_context.pow_owner) {
    case NO_OWNER:
      // TO change with the value that you don't want to follow with scroll.
      return false;
    case DIAGNOSTICS:
      slice_x = 0;
    default:
      break;
  }

  if (slice_x == 0 && slice_y == 0) {
    return true;
  }

  const int x = getbegx(gui_context->edw_context.pow) - slice_x;
  const int y = getbegy(gui_context->edw_context.pow) - slice_y;
  const int width = getmaxx(gui_context->edw_context.pow);
  const int height = getmaxy(gui_context->edw_context.pow);

  delwin(gui_context->edw_context.pow);
  gui_context->edw_context.pow = newwin(height, width, y, x);

  gui_context->edw_context.show_pow = gui_context->edw_context.pow != NULL;
  if (gui_context->edw_context.pow) {
    updateEDW(gui_context);
  }
  else {
    gui_context->edw_context.pow_owner = NO_OWNER;
  }

  return gui_context->edw_context.show_pow;
}

void gui_closePopup(GUIContext* gui_context) {
  gui_context->edw_context.show_pow = false;
  gui_context->edw_context.pow_owner = NO_OWNER;
  updateEDW(gui_context);
}
