#include "editor_input.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ttydefaults.h>
#include <time.h>

#include "../advanced/lsp/lsp-features/lsp_completion.h"
#include "../data-management/state_control.h"
#include "../environnement/global-variables.h"
#include "../io-management/io_manager.h"
#include "../terminal/click_handler.h"
#include "../terminal/windows/few.h"
#include "../terminal/windows/ofw.h"
#include "../terminal/windows/pow.h"
#include "../utils/clipboard_manager.h"
#include "../utils/key_management.h"


bool handlePopupInput(EditorContext* ctx, int c, int hash, ModuleContext* payload) {
  FileContainer* fc = &ctx->files[ctx->current_file_index];
  MEVENT* mouse_event = (hash == KEY_MOUSE) ? &ctx->m_event : NULL;
  return gui_handlePopupInput(&ctx->gui_context, fc, c, hash, ctx->payload_state_change, payload, mouse_event);
}

void readNextInput(EditorContext* ctx, int* out_c, int* out_hash) {
  int c;
  if (ctx->peek_c == -1) {
    c = getch();
  }
  else {
    c = ctx->peek_c;
    ctx->peek_c = -1;
  }
  int hash = c;

  ctx->t_date = timeInMilliseconds();
  ctx->t_clock = clock();

  if (c != KEY_MOUSE && c != -1) {
    fprintf(stderr, "Code %d, Key : '%s' hash into %d.\n", c, keyname(c), hashString((unsigned char*)keyname(c)));
    const char* key_str = keyname(c);
    if (key_str != NULL && key_str[0] != '\0') {
      if (key_str[0] != '^') {
        hash = hashString((unsigned char*)key_str);
      }
    }
    else {
      fprintf(stderr, "keyname is NULL");
    }
  }

  *out_c = c;
  *out_hash = hash;
}

EventLoopAction runKeyHandler(EditorContext* ctx, int c, int hash) {
  FileContainer* fc = &ctx->files[ctx->current_file_index];

  Cursor* cursor = &fc->cursor;
  Cursor* select_cursor = &fc->select_cursor;
  int* desired_column = &fc->desired_column;
  int* screen_x = &fc->screen_x;
  int* screen_y = &fc->screen_y;
  History** history_frame = &fc->history_frame;
  History** history_root = &fc->history_root;
  IO_FileID* io_file = &fc->io_file;
  FileNode** root = &fc->root;
  LSP_Data* lsp_data = &fc->lsp_datas;
  CursorDescriptor tmp;

  switch (hash) {
    case ONLY_REPAINT_INPUT:
      gui_updateGUI(&ctx->gui_context);
      break;

    case BEGIN_MOUSE_LISTEN:
    case MOUSE_IN_OUT:
    case H_KEY_RESIZE:
      gui_resizeOFW(&ctx->gui_context);
      gui_resizeEDW(&ctx->gui_context, -1);
      gui_updateGUI(&ctx->gui_context);
      break;

    case KEY_MOUSE:
      handleClick(ctx, &c);
      break;

    case H_KEY_RIGHT:
      if (cursor_is_disabled(*select_cursor)) {
        *cursor = moveRight(*cursor);
      }
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      break;
    case H_KEY_LEFT:
      if (cursor_is_disabled(*select_cursor)) {
        *cursor = moveLeft(*cursor);
      }
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_LEFT);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      break;
    case H_KEY_UP:
      *cursor = moveUp(*cursor, *desired_column);
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_LEFT);
      break;
    case H_KEY_DOWN:
      *cursor = moveDown(*cursor, *desired_column);
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
      break;
    case H_KEY_MAJ_RIGHT:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveRight(*cursor);
      setDesiredColumn(*cursor, desired_column);
      break;
    case H_KEY_MAJ_LEFT:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveLeft(*cursor);
      setDesiredColumn(*cursor, desired_column);
      break;
    case H_KEY_MAJ_UP:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveUp(*cursor, *desired_column);
      break;
    case H_KEY_MAJ_DOWN:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveDown(*cursor, *desired_column);
      break;
    case H_KEY_CTRL_RIGHT:
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
      *cursor = moveToNextWord(*cursor);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, true, false);
      break;
    case H_KEY_CTRL_LEFT:
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_LEFT);
      *cursor = moveToPreviousWord(*cursor);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, true, false);
      break;
    case H_KEY_CTRL_DOWN:
    case H_KEY_CTRL_UP:
      selectWord(cursor, select_cursor);
      break;
    case H_KEY_CTRL_MAJ_RIGHT:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveToNextWord(*cursor);
      setDesiredColumn(*cursor, desired_column);
      break;
    case H_KEY_CTRL_MAJ_LEFT:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveToPreviousWord(*cursor);
      setDesiredColumn(*cursor, desired_column);
      break;
    case H_KEY_CTRL_MAJ_DOWN:
      if (ctx->current_file_index != 0) {
        ctx->current_file_index--;
      }
      ctx->refresh_local_vars = true;
      gui_updateOFW(&ctx->gui_context);
      break;
    case H_KEY_CTRL_MAJ_UP:
      if (ctx->current_file_index != ctx->file_count - 1) {
        ctx->current_file_index++;
      }
      ctx->refresh_local_vars = true;
      gui_updateOFW(&ctx->gui_context);
      break;
    case H_KEY_BEGIN:
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_LEFT);
      *cursor = goToBegin(*cursor);
      setDesiredColumn(*cursor, desired_column);
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
      *cursor = moveToNextWord(*cursor);
      setDesiredColumn(*cursor, desired_column);
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_LEFT);
      *cursor = moveToPreviousWord(*cursor);
      setDesiredColumn(*cursor, desired_column);
      break;
    case H_KEY_END:
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
      *cursor = goToEnd(*cursor);
      setDesiredColumn(*cursor, desired_column);
      break;
    case H_KEY_MAJ_END:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = goToEnd(*cursor);
      setDesiredColumn(*cursor, desired_column);
      break;
    case CTRL('z'):
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_LEFT);
      *cursor = undo(history_frame, *cursor, globalOnStageChange, (long*)&ctx->payload_state_change);
      ctx->old_history_frame = NULL;
      setDesiredColumn(*cursor, desired_column);
      gui_updateEDW(&ctx->gui_context);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, true, false);
      break;
    case CTRL('y'):
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_LEFT);
      *cursor = redo(history_frame, *cursor, globalOnStageChange, (long*)&ctx->payload_state_change);
      ctx->old_history_frame = NULL;
      setDesiredColumn(*cursor, desired_column);
      gui_updateEDW(&ctx->gui_context);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, true, false);
      break;
    case CTRL('c'):
      saveToClipBoard(*cursor, *select_cursor);
      break;
    case CTRL('a'):
      *select_cursor = tryToReachAbsPosition(*cursor, 1, 0);
      *cursor = tryToReachAbsPosition(*cursor, INT_MAX, INT_MAX);
      break;
    case CTRL('x'):
      saveToClipBoard(*cursor, *select_cursor);
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      gui_updateEDW(&ctx->gui_context);
      break;
    case CTRL('v'):
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      tmp = cursor_to_desc(*cursor);
      *cursor = loadFromClipBoard(*cursor);
      saveAction(history_frame, createInsertAction(*cursor, tmp), globalOnStageChange, cursor,
                 (long*)&ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      break;
    case CTRL('q'):
      return EVENT_QUIT;
    case CTRL('w'):
      closeFile(&ctx->files, &ctx->file_count, &ctx->current_file_index, &ctx->refresh_local_vars);
      gui_updateOFW(&ctx->gui_context);
      gui_updateEDW(&ctx->gui_context);
      break;
    case CTRL('s'):
      if (io_file->status == NONE) {
        printf("\r\nNo specified file\r\n");
        return EVENT_QUIT;
      }
      saveFile(*root, io_file);
      assert(io_file->status == EXIST);
      setlastFilePosition(io_file->path_abs, cursor_row(*cursor), cursor_col(*cursor), *screen_x, *screen_y);
      saveCurrentStateControl(**history_root, *history_frame, io_file->path_abs);
      break;
    case H_KEY_CTRL_DELETE:
    case CTRL('H'):
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveToPreviousWord(*cursor);
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      break;
    case '\n':
    case KEY_ENTER:
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      tmp = cursor_to_desc(*cursor);
      *cursor = insertNewLineInLineC(*cursor);
      saveAction(history_frame, createInsertAction(*cursor, tmp), globalOnStageChange, cursor,
                 (long*)&ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      break;
    case H_KEY_DELETE:
      if (cursor_is_disabled(*select_cursor)) {
        *select_cursor = moveLeft(*cursor);
      }
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      break;
    case H_KEY_SUPPR:
      if (cursor_is_disabled(*select_cursor)) {
        *select_cursor = moveRight(*cursor);
      }
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      gui_updateEDW(&ctx->gui_context);
      break;
    case H_KEY_CTRL_SUPPR:
      setSelectCursorOn(*cursor, select_cursor);
      *cursor = moveToNextWord(*cursor);
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      gui_updateEDW(&ctx->gui_context);
      break;
    case KEY_TAB:
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      tmp = cursor_to_desc(*cursor);
      if (TAB_CHAR_USE) {
        *cursor = insertCharInLineC(*cursor, readChar_U8FromInput('\t'));
      }
      else {
        for (int i = 0; i < TAB_SIZE; i++) {
          *cursor = insertCharInLineC(*cursor, readChar_U8FromInput(' '));
        }
      }
      saveAction(history_frame, createInsertAction(*cursor, tmp), globalOnStageChange, cursor,
                 (long*)&ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      break;
    case CTRL('d'):
      if (cursor_is_disabled(*select_cursor) == true) {
        selectLine(cursor, select_cursor);
      }
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      gui_closePopup(&ctx->gui_context);
      gui_updateEDW(&ctx->gui_context);
      break;
    case CTRL('e'):
      switchFEW(&ctx->gui_context);
      break;
    case CTRL('l'):
      gui_switchOFW(&ctx->gui_context);
      break;
    case CTRL(' '):
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, true, true);
      break;
    case H_KEY_ESCAPE:
    case CTRL('['):
      gui_closePopup(&ctx->gui_context);
      break;
    default:
      if (iscntrl(c)) {
        printf("Unsupported touch %d", c);
      }
      else {
        deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
        tmp = cursor_to_desc(*cursor);
        *cursor = insertCharInLineC(*cursor, readChar_U8FromInput(c));
        setDesiredColumn(*cursor, desired_column);
        saveAction(history_frame, createInsertAction(*cursor, tmp), globalOnStageChange, cursor,
                   (long*)&ctx->payload_state_change);
        if (ctx->gui_context.edw_context.pow_owner == DIAGNOSTICS) {
          gui_closePopup(&ctx->gui_context);
        }
        askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      }
      break;
  }
  return EVENT_CONTINUE;
}
