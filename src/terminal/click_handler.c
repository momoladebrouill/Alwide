#include "click_handler.h"

#include <assert.h>
#include <libgen.h>
#include <string.h>
#include <time.h>

#include "../data-management/file_management.h"
#include "../utils/constants.h"
#include "../utils/key_management.h"
#include "term_handler.h"
#include "windows/edw.h"
#include "windows/few.h"
#include "windows/pow.h"


bool handleClick(GUIContext* gui_context, FileContainer** files, int* file_count, int* current_file_index,
                 ExplorerFolder* pwd, Cursor* cursor, Cursor* select_cursor, int* desired_column, int* screen_x,
                 int* screen_y, bool* refresh_local_vars, MEVENT* m_event, int* peek_c, bool* mouse_drag,
                 time_val* last_time_mouse_drag, time_val* t_date, clock_t* t_clock, int* c,
                 WindowHighlightDescriptor* highlight_descriptor) {
mouse_read:;
  assert(mouse_drag != NULL);

  detectComplexMouseEvents(m_event);


  // Avoid too much refresh, to avoid input buffer full.
  if (m_event->bstate == NO_EVENT_MOUSE && *mouse_drag == true) {
    time_val current_time = timeInMilliseconds();
    if (diff2Time(*last_time_mouse_drag, current_time) < SKIP_MOUSE_EVENT_DELAY) {
      *peek_c = getch();
      if (*peek_c != ERR && *peek_c == KEY_MOUSE) {
        MEVENT tmp_event;
        if (getmouse(&tmp_event) == OK) {
          // skip current event.
          *c = *peek_c;
          *peek_c = -1;
          *m_event = tmp_event;
          *t_date = timeInMilliseconds();
          *t_clock = clock();
          goto mouse_read;
        }
        else {
          return false;
        }
      }
    }
    *last_time_mouse_drag = current_time;
  }

  bool force_repaint = false;
  // If pressed enable drag
  if (m_event->bstate & BUTTON1_PRESSED && *mouse_drag == false) {
    *mouse_drag = true;
  }

  if ((m_event->x < getbegx(gui_context->edw_context.lnw) && gui_context->focus_w == NULL) ||
      (gui_context->few_context.few != NULL && gui_context->focus_w == gui_context->few_context.few)) {
    // Click in File Explorer Window
    if (m_event->bstate & BUTTON1_PRESSED) {
      gui_context->focus_w = gui_context->few_context.few;
    }
    handleFileExplorerClick(gui_context, files, file_count, current_file_index, pwd, *m_event, refresh_local_vars);
  }
  else if ((m_event->y - gui_context->ofw_context.ofw_height < 0 && gui_context->focus_w == NULL) ||
           (gui_context->ofw_context.ofw != NULL && gui_context->focus_w == gui_context->ofw_context.ofw)) {
    // Click on opened file window
    if (m_event->bstate & BUTTON1_PRESSED) {
      gui_context->focus_w = gui_context->ofw_context.ofw;
    }
    handleOpenedFileClick(gui_context, *files, file_count, current_file_index, *m_event, refresh_local_vars,
                          *mouse_drag);
  }
  else {
    // Click on editor windows
    if (m_event->bstate & BUTTON1_PRESSED) {
      gui_context->focus_w = gui_context->edw_context.ftw;
    }
    force_repaint = handleEditorClick(gui_context, cursor, select_cursor, desired_column, screen_x, screen_y, m_event,
                                      *mouse_drag, highlight_descriptor);
  }

  if (m_event->bstate & BUTTON1_RELEASED || m_event->bstate & BUTTON1_CLICKED) {
    gui_context->focus_w = NULL;
    *mouse_drag = false;
  }

  // Avoid refreshing when it's just mouse movement with no change.
  if (m_event->bstate == NO_EVENT_MOUSE /*No event state*/ && *mouse_drag == false && force_repaint == false) {
    return false;
  }
  return true;
}


////// -------------- CLICK FUNCTIONS --------------


bool handleEditorClick(GUIContext* gui_context, Cursor* cursor, Cursor* select_cursor, int* desired_column,
                       int* screen_x, int* screen_y, MEVENT* m_event, bool mouse_drag,
                       WindowHighlightDescriptor* highlight_descriptor) {
  int edws_offset_x = getbegx(gui_context->edw_context.ftw);
  int edws_offset_y = gui_context->ofw_context.ofw_height;
  if (m_event->y - edws_offset_y < 0) {
    m_event->y = edws_offset_y;
  }


  // ---------- CURSOR ACTION ------------

  if (mouse_drag) {
    FileIdentifier new_file_id = tryToReachAbsRow(cursor->file_id, *screen_y + m_event->y - edws_offset_y);
    LineIdentifier new_line_id = getLineIdForScreenX(moduloLineIdentifierR(getLineForFileIdentifier(new_file_id), 0),
                                                     *screen_x, m_event->x - edws_offset_x);

    if (isCursorDisabled(*select_cursor) == false) {
      *cursor = cursorOf(new_file_id, new_line_id);
    }
    else if (areCursorEqual(*cursor, cursorOf(new_file_id, new_line_id)) == false) {
      *select_cursor = *cursor;
      *cursor = cursorOf(new_file_id, new_line_id);
    }
    setDesiredColumn(*cursor, desired_column);
  }

  if (m_event->bstate & BUTTON1_PRESSED) {
    setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
    FileIdentifier new_file_id = tryToReachAbsRow(cursor->file_id, *screen_y + m_event->y - edws_offset_y);
    LineIdentifier new_line_id = getLineIdForScreenX(moduloLineIdentifierR(getLineForFileIdentifier(new_file_id), 0),
                                                     *screen_x, m_event->x - edws_offset_x);
    *cursor = cursorOf(new_file_id, new_line_id);
    setDesiredColumn(*cursor, desired_column);
  }

  if (m_event->bstate & BUTTON1_DOUBLE_CLICKED) {
    selectWord(cursor, select_cursor);
  }


  // ---------- SCROLL ------------
  if (m_event->bstate & BUTTON4_PRESSED && !(m_event->bstate & BUTTON_SHIFT)) {
    // Move Up
    *screen_y -= SCROLL_SPEED;
    if (*screen_y < 1)
      *screen_y = 1;
  }
  else if (m_event->bstate & BUTTON4_PRESSED && m_event->bstate & BUTTON_SHIFT) {
    // Move Left
    *screen_x -= SCROLL_SPEED;
    if (*screen_x < 1)
      *screen_x = 1;
  }

  if (m_event->bstate & BUTTON5_PRESSED && !(m_event->bstate & BUTTON_SHIFT)) {
    // Move Down
    *screen_y += SCROLL_SPEED;
    FileIdentifier last_line_cur = tryToReachAbsRow(cursor->file_id, *screen_y + 2);
    if (*screen_y + 2 != last_line_cur.absolute_row) {
      *screen_y = last_line_cur.absolute_row - 2;
      if (*screen_y < 1)
        *screen_y = 1;
    }
  }
  else if (m_event->bstate & BUTTON5_PRESSED && m_event->bstate & BUTTON_SHIFT) {
    // Move Right
    *screen_x += SCROLL_SPEED;
  }

  if (getbegx(gui_context->edw_context.lnw) <= m_event->x &&
      m_event->x < getbegx(gui_context->edw_context.lnw) + getmaxx(gui_context->edw_context.lnw)) {

    Diagnostic* diagnostic;
    getMarkerForCurrentLine(*screen_y + m_event->y - getbegy(gui_context->edw_context.lnw), highlight_descriptor, 0,
                            &diagnostic);

    gui_showDiagnostic(gui_context, m_event->y - getbegy(gui_context->edw_context.lnw),
                       getbegy(gui_context->edw_context.ftw), diagnostic);

    if (diagnostic != NULL) {
      return true;
    }
  }
  else if (gui_context->edw_context.show_pow == true && gui_context->edw_context.pow_owner == DIAGNOSTICS) {
    gui_closePopup(gui_context);
    return true;
  }

  return false;
}

int handleOpenedFileSelectClick(GUIContext* gui_context, FileContainer* files, int* file_count, int* current_file,
                                MEVENT m_event) {
  OFW_GUIContext* ofw_content = &gui_context->ofw_context;

  // Char offset for the window
  int current_char_offset = getbegx(ofw_content->ofw);

  if (ofw_content->current_file_offset != 0) {
    current_char_offset += strlen("< | ");
  }

  for (int i = ofw_content->current_file_offset; i < *file_count; i++) {
    current_char_offset += strlen(basename(files[i].io_file.path_args));

    if (ofw_content->current_file_offset != 0 && m_event.x < getbegx(ofw_content->ofw) + 3) {
      ofw_content->current_file_offset--;
      assert(ofw_content->current_file_offset >= 0);
      ofw_content->refresh_ofw = true;
      break;
    }

    if (current_char_offset + strlen(FILE_NAME_SEPARATOR) > COLS && m_event.x > COLS - 4) {
      ofw_content->current_file_offset++;
      assert(ofw_content->current_file_offset < *file_count);
      ofw_content->refresh_ofw = true;
      break;
    }

    if (m_event.x < current_char_offset + (strlen(FILE_NAME_SEPARATOR) / 2)) {
      // Don't change anything if the file clicked is currently the file opened.
      if (*current_file == i)
        break;

      return i;
    }
    current_char_offset += strlen(FILE_NAME_SEPARATOR);
  }

  return -1;
}


void handleOpenedFileClick(GUIContext* gui_context, FileContainer* files, int* file_count, int* current_file,
                           MEVENT m_event, bool* refresh_local_vars, bool mouse_drag) {
  OFW_GUIContext* ofw_context = &gui_context->ofw_context;
  if (m_event.bstate & BUTTON4_PRESSED && !(m_event.bstate & BUTTON_SHIFT)) {
    // Move Up
    (ofw_context->current_file_offset)--;
    if (ofw_context->current_file_offset < 0)
      ofw_context->current_file_offset = 0;
    ofw_context->refresh_ofw = true;
    return;
  }
  if (m_event.bstate & BUTTON5_PRESSED && !(m_event.bstate & BUTTON_SHIFT)) {
    // Move Down
    (ofw_context->current_file_offset)++;
    if (ofw_context->current_file_offset > *file_count - 1)
      ofw_context->current_file_offset = *file_count - 1;
    ofw_context->refresh_ofw = true;
    return;
  }


  if (m_event.bstate & BUTTON1_PRESSED) {
    int result_ofw_click = handleOpenedFileSelectClick(gui_context, files, file_count, current_file, m_event);

    if (result_ofw_click == -1) {
      return;
    }

    *current_file = result_ofw_click;

    *refresh_local_vars = true;
    ofw_context->refresh_ofw = true;
    return;
  }

  // ReOrder opened Files.
  if (m_event.bstate == NO_EVENT_MOUSE && mouse_drag == true) {
    int result_ofw_click = handleOpenedFileSelectClick(gui_context, files, file_count, current_file, m_event);

    if (result_ofw_click == -1 || result_ofw_click == *current_file) {
      return;
    }

    // Swap both files.
    FileContainer tmp = files[result_ofw_click];
    files[result_ofw_click] = files[*current_file];
    files[*current_file] = tmp;

    *current_file = result_ofw_click;

    *refresh_local_vars = true;
    ofw_context->refresh_ofw = true;
    return;
  }
}

void handleFileExplorerClick(GUIContext* gui_context, FileContainer** files, int* file_count, int* current_file,
                             ExplorerFolder* pwd, MEVENT m_event, bool* refresh_local_vars) {
  FEW_GUIContext* few_context = &gui_context->few_context;

  // ---------- SCROLL ------------
  if (m_event.bstate & BUTTON4_PRESSED && !(m_event.bstate & BUTTON_SHIFT)) {
    // Move Up
    few_context->few_y_offset -= SCROLL_SPEED;
    if (few_context->few_y_offset < 0)
      few_context->few_y_offset = 0;
  }
  else if (m_event.bstate & BUTTON4_PRESSED && m_event.bstate & BUTTON_SHIFT) {
    // Move Left
    gui_resizeFEW(gui_context, few_context->few_width - 1);
  }

  if (m_event.bstate & BUTTON5_PRESSED && !(m_event.bstate & BUTTON_SHIFT)) {
    // Move Down
    few_context->few_y_offset += SCROLL_SPEED;
  }
  else if (m_event.bstate & BUTTON5_PRESSED && m_event.bstate & BUTTON_SHIFT) {
    // Move Right
    if (few_context->few_width < COLS - 8) {
      gui_resizeFEW(gui_context, few_context->few_width + 1);
    }
  }

  if (m_event.bstate & BUTTON1_PRESSED) {
    few_context->few_selected_line = few_context->few_y_offset + m_event.y + 1;
    few_context->refresh_few = true;
  }

  if (!(m_event.bstate & BUTTON1_DOUBLE_CLICKED))
    return;

  ExplorerFolder* res_folder;
  int res_index;
  bool found = getFileClickedFileExplorer(pwd, m_event.y, few_context->few_x_offset, few_context->few_y_offset,
                                          &res_folder, &res_index);
  few_context->few_selected_line = few_context->few_y_offset + m_event.y + 1;
  // If click on nothing break;
  if (found == false) {
    return;
  }

  if (res_index == -1) {
    // Result is a folder
    // switch open.
    res_folder->open = !res_folder->open;
  }
  else {
    // Result is a file => open file in text editor.
    openNewFile(res_folder->files[res_index].path, files, file_count, current_file,
                &gui_context->ofw_context.refresh_ofw, refresh_local_vars);
  }

  few_context->refresh_few = true;
}


bool internalGetClickedExplorerFile(ExplorerFolder* current_folder, int* y_click, int few_x_offset, int few_y_offset,
                                    ExplorerFolder** res_folder, int* file_index) {
  if (*y_click == 0) {
    *res_folder = current_folder;
    *file_index = -1;
    return true;
  }
  (*y_click)--;

  // Don't check childs if current_folder isn't opened.
  if (current_folder->open == false)
    return false;

  // Check for child folders click.
  for (int i = 0; i < current_folder->folder_count; i++) {
    if (internalGetClickedExplorerFile(current_folder->folders + i, y_click, few_x_offset, few_y_offset, res_folder,
                                       file_index) == true) {
      return true;
    }
  }

  // Check for child file click.
  for (int i = 0; i < current_folder->file_count; i++) {
    if (*y_click == 0) {
      *res_folder = current_folder;
      *file_index = i;
      return true;
    }
    (*y_click)--;
  }

  return false;
}

bool getFileClickedFileExplorer(ExplorerFolder* pwd, int y_click, int few_x_offset, int few_y_offset,
                                ExplorerFolder** res_folder, int* file_index) {
  y_click += few_y_offset;
  return internalGetClickedExplorerFile(pwd, &y_click, few_x_offset, few_y_offset, res_folder, file_index);
}
