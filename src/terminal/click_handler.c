#include "click_handler.h"

#include <assert.h>
#include <libgen.h>
#include <string.h>
#include <time.h>

#include "../data-management/file_management.h"
#include "../environnement/constants.h"
#include "../environnement/global_variables.h"
#include "../utils/key_management.h"
#include "term_handler.h"
#include "windows/edw.h"
#include "windows/few.h"
#include "windows/pow.h"
#include "../advanced/lsp/lsp-features/lsp_code_action.h"


bool isClickInsideWindow(WINDOW* w, MEVENT* m_event) {
  return getbegx(w) <= m_event->x && m_event->x < getbegx(w) + getmaxx(w) && getbegy(w) <= m_event->y &&
    m_event->y < getbegy(w) + getmaxy(w);
}

Cursor getCursorForEDWClick(Cursor* cursor, MEVENT* m_event, int screen_x, int screen_y, int edws_offset_x,
                            int edws_offset_y) {
  FileIdentifier new_file_id = tryToReachAbsRow(cursor->file_id, screen_y + m_event->y - edws_offset_y);
  LineIdentifier new_line_id = getLineIdForScreenX(moduloLineIdentifierR(getLineForFileIdentifier(new_file_id), 0),
                                                   screen_x, m_event->x - edws_offset_x);

  return cursorOf(new_file_id, new_line_id);
}


void handleClick(EditorContext* ctx, int* c) {
mouse_read:;
  assert(&ctx->mouse_drag != NULL);

  // Avoid too much refresh, to avoid input buffer full.
  // We drain pending mouse events if they arrive too fast.
  if (ctx->m_event.bstate == NO_EVENT_MOUSE && ctx->mouse_drag == true) {
    time_val current_time = timeInMilliseconds();
    if (diff2Time(ctx->last_time_mouse_drag, current_time) < SKIP_MOUSE_EVENT_DELAY) {
      nodelay(stdscr, TRUE);
      int next_c = getch();
      if (next_c != ERR && next_c == KEY_MOUSE) {
        MEVENT tmp_event;
        if (getmouse(&tmp_event) == OK) {
          // skip current event but process its state change
          detectComplexMouseEvents(&tmp_event);
          *c = next_c;
          ctx->m_event = tmp_event;
          ctx->t_date = timeInMilliseconds();
          ctx->t_clock = clock();
          nodelay(stdscr, FALSE);
          goto mouse_read;
        }
      }
      if (next_c != ERR) {
        ctx->peek_c = next_c;
      }
      nodelay(stdscr, FALSE);
    }
    else {
      ctx->last_time_mouse_drag = current_time;
    }
  }

  // If pressed enable drag
  if (ctx->m_event.bstate & BUTTON1_PRESSED && ctx->mouse_drag == false) {
    ctx->mouse_drag = true;
  }

  FileContainer* fc = &ctx->files[ctx->current_file_index];

  if ((ctx->m_event.x < getbegx(ctx->gui_context.edw_context.lnw) && ctx->gui_context.focus_w == NULL) ||
      (ctx->gui_context.few_context.few != NULL && ctx->gui_context.focus_w == ctx->gui_context.few_context.few)) {
    // Click in File Explorer Window
    if (ctx->m_event.bstate & BUTTON1_PRESSED) {
      ctx->gui_context.focus_w = ctx->gui_context.few_context.few;
    }
    handleFileExplorerClick(&ctx->gui_context, &ctx->files, &ctx->file_count, &ctx->current_file_index, &ctx->pwd, ctx->m_event, &ctx->refresh_local_vars);
  }
  else if ((ctx->m_event.y - ctx->gui_context.ofw_context.ofw_height < 0 && ctx->gui_context.focus_w == NULL) ||
           (ctx->gui_context.ofw_context.ofw != NULL && ctx->gui_context.focus_w == ctx->gui_context.ofw_context.ofw)) {
    // Click on opened file window
    if (ctx->m_event.bstate & BUTTON1_PRESSED) {
      ctx->gui_context.focus_w = ctx->gui_context.ofw_context.ofw;
    }
    handleOpenedFileClick(&ctx->gui_context, ctx->files, &ctx->file_count, &ctx->current_file_index, ctx->m_event, &ctx->refresh_local_vars,
                          ctx->mouse_drag);
  }
  else {
    // Click on editor windows
    if (ctx->m_event.bstate & BUTTON1_PRESSED) {
      ctx->gui_context.focus_w = ctx->gui_context.edw_context.ftw;
    }
    handleEditorClick(&ctx->gui_context, &fc->cursor, &fc->select_cursor, &fc->desired_column, &fc->screen_x, &fc->screen_y, &ctx->m_event, ctx->mouse_drag,
                      fc, &ctx->highlight_descriptor);
  }

  if (ctx->m_event.bstate & BUTTON1_RELEASED || ctx->m_event.bstate & BUTTON1_CLICKED) {
    ctx->gui_context.focus_w = NULL;
    ctx->mouse_drag = false;
  }
}


////// -------------- CLICK FUNCTIONS --------------


void handleEditorClick(GUIContext* gui_context, Cursor* cursor, Cursor* select_cursor, int* desired_column,
                       int* screen_x, int* screen_y, MEVENT* m_event, bool mouse_drag, FileContainer* file,
                       WindowHighlightDescriptor* highlight_descriptor) {
  int edws_offset_x = getbegx(gui_context->edw_context.ftw);
  int edws_offset_y = getbegy(gui_context->edw_context.ftw);
  if (m_event->y - edws_offset_y < 0) {
    m_event->y = edws_offset_y;
  }


  // ---------- CURSOR ACTION ------------

  if (mouse_drag) {
    FileIdentifier new_file_id = tryToReachAbsRow(cursor->file_id, *screen_y + m_event->y - edws_offset_y);
    LineIdentifier new_line_id = getLineIdForScreenX(moduloLineIdentifierR(getLineForFileIdentifier(new_file_id), 0),
                                                     *screen_x, m_event->x - edws_offset_x);

    if (cursor_is_disabled(*select_cursor) == false) {
      *cursor = cursorOf(new_file_id, new_line_id);
    }
    else if (cursor_ne(*cursor, cursorOf(new_file_id, new_line_id))) {
      *select_cursor = *cursor;
      *cursor = cursorOf(new_file_id, new_line_id);
    }
    setDesiredColumn(*cursor, desired_column);
    gui_closePopup(gui_context);
  }

  if (m_event->bstate & BUTTON1_PRESSED) {
    setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
    *cursor = getCursorForEDWClick(cursor, m_event, *screen_x, *screen_y, edws_offset_x, edws_offset_y);
    setDesiredColumn(*cursor, desired_column);
    gui_closePopup(gui_context);
  }

  if (m_event->bstate & BUTTON1_DOUBLE_CLICKED) {
    selectWord(cursor, select_cursor);
    gui_closePopup(gui_context);
  }


  // ---------- SCROLL ------------
  if (m_event->bstate & BUTTON4_PRESSED && !(m_event->bstate & BUTTON_SHIFT)) {
    // Move Up
    *screen_y -= SCROLL_SPEED;
    if (*screen_y < 1)
      *screen_y = 1;
    gui_updateEDW(gui_context);
  }
  else if (m_event->bstate & BUTTON4_PRESSED && m_event->bstate & BUTTON_SHIFT) {
    // Move Left
    *screen_x -= SCROLL_SPEED;
    if (*screen_x < 1)
      *screen_x = 1;
    gui_updateEDW(gui_context);
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
    gui_updateEDW(gui_context);
  }
  else if (m_event->bstate & BUTTON5_PRESSED && m_event->bstate & BUTTON_SHIFT) {
    // Move Right
    *screen_x += SCROLL_SPEED;
    gui_updateEDW(gui_context);
  }

  // line number marker.
  if (isClickInsideWindow(gui_context->edw_context.lnw, m_event)) {
    LSP_Diagnostic* diagnostic;
    LineMarker marker = gui_getMarkerForCurrentLine(*screen_y + m_event->y - getbegy(gui_context->edw_context.lnw),
                                                    highlight_descriptor, 0, (void**)&diagnostic);

    if ((marker == LSP_ERROR || marker == LSP_HINT || marker == LSP_INFORMATION || marker == LSP_WARNING) &&
        diagnostic != NULL) {
      if (m_event->bstate & BUTTON1_DOUBLE_CLICKED) {
          askCodeAction(file, cursor);
      } else {
          gui_showDiagnostic(gui_context, m_event->y - getbegy(gui_context->edw_context.lnw) + 1,
                         getbegy(gui_context->edw_context.ftw), diagnostic);
      }
    }
  }
  else if (isClickInsideWindow(gui_context->edw_context.ftw, m_event)) {
    // track mouse position
    // TODO may safe raw m_event position to avoid calling @getCursorForEDWCLick() everytime which is heavy.
    Cursor hover_cursor = getCursorForEDWClick(cursor, m_event, *screen_x, *screen_y, edws_offset_x, edws_offset_y);
    gui_context->edw_context.lastMousePosition = cursor_to_desc(hover_cursor);
    if (file->lsp_datas.is_enable && m_event->bstate & BUTTON_CTRL) {

      // goto action ctrl + click
      if (m_event->bstate & BUTTON1_CLICKED) {
        LSP_requestGoto(getLSPServerForLanguage(&lsp_servers, file->lsp_datas.lang_id), file->io_file.path_abs,
                        LSP_pos_from_cursor(cursor_row(hover_cursor), cursor_col(hover_cursor)), LSP_GOTO_DEFINITION);
      }
      if (gui_context->edw_context.pow_owner != GOTO_CHOICE) {
        if (file->lsp_datas.computed->hover.is_range == false ||
            !cursor_desc_is_between(cursor_to_desc(hover_cursor),
                                    positionToCursorDescriptor(file->lsp_datas.computed->hover.range.pos1),
                                    positionToCursorDescriptor(file->lsp_datas.computed->hover.range.pos2))) {
          // Regulate the hover requests, to avoid spamming for nothing. Don't reask for the same word.
          LSP_requestHover(getLSPServerForLanguage(&lsp_servers, file->lsp_datas.lang_id), file->io_file.path_abs,
                           LSP_pos_from_cursor(cursor_row(hover_cursor), cursor_col(hover_cursor)));
        }
        else if (file->lsp_datas.computed->hover.size != 0) {
          // We resume the hover data previously fetched.
          ViewPort view_port = (ViewPort){.gui = gui_context, .screen_x = screen_x, .screen_y = screen_y};
          gui_resumeHoverInformation(cursor, &view_port, &file->lsp_datas.computed->hover);
        }
      }
    }
  }
  else if (gui_context->edw_context.show_pow == true &&
           (gui_context->edw_context.pow_owner == DIAGNOSTICS ||
            gui_context->edw_context.pow_owner == HOVER_DIAGNOSTICS)) {
    gui_closePopup(gui_context);
  }
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
    gui_updateOFW(gui_context);
    return;
  }

  if (m_event.bstate & BUTTON5_PRESSED && !(m_event.bstate & BUTTON_SHIFT)) {
    // Move Down
    (ofw_context->current_file_offset)++;
    if (ofw_context->current_file_offset > *file_count - 1)
      ofw_context->current_file_offset = *file_count - 1;
    gui_updateOFW(gui_context);
    return;
  }


  if (m_event.bstate & BUTTON1_PRESSED) {
    int result_ofw_click = handleOpenedFileSelectClick(gui_context, files, file_count, current_file, m_event);

    if (result_ofw_click == -1) {
      return;
    }

    *current_file = result_ofw_click;

    *refresh_local_vars = true;
    gui_updateOFW(gui_context);
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
    gui_updateOFW(gui_context);
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
    gui_updateFEW(gui_context);
  }
  else if (m_event.bstate & BUTTON4_PRESSED && m_event.bstate & BUTTON_SHIFT) {
    // Move Left
    gui_resizeFEW(gui_context, few_context->few_width - 1);
  }

  if (m_event.bstate & BUTTON5_PRESSED && !(m_event.bstate & BUTTON_SHIFT)) {
    // Move Down
    few_context->few_y_offset += SCROLL_SPEED;
    gui_updateFEW(gui_context);
  }
  else if (m_event.bstate & BUTTON5_PRESSED && m_event.bstate & BUTTON_SHIFT) {
    // Move Right
    if (few_context->few_width < COLS - 8) {
      gui_resizeFEW(gui_context, few_context->few_width + 1);
    }
  }

  if (m_event.bstate & BUTTON1_PRESSED) {
    few_context->few_selected_line = few_context->few_y_offset + m_event.y + 1;
    gui_updateFEW(gui_context);
  }

  if (!(m_event.bstate & BUTTON1_DOUBLE_CLICKED)) {
    return;
  }

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

  gui_updateFEW(gui_context);
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
