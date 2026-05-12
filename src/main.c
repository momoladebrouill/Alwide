#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ttydefaults.h>
#include <time.h>
#include <unistd.h>

#include "advanced/lsp/lsp_client.h"
#include "advanced/lsp/lsp_dispatcher.h"
#include "advanced/lsp/lsp_features/lsp_completion.h"
#include "advanced/lsp/lsp_highlighter.h"
#include "advanced/tree-sitter/tree_manager.h"
#include "advanced/tree-sitter/tree_sitter_highlighter.h"
#include "config/config.h"
#include "data-management/file_management.h"
#include "data-management/file_structure.h"
#include "data-management/state_control.h"
#include "environnement/constants.h"
#include "environnement/global-variables.h"
#include "io_management/io_explorer.h"
#include "io_management/io_manager.h"
#include "io_management/viewport_history.h"
#include "io_management/workspace_settings.h"
#include "terminal/click_handler.h"
#include "terminal/highlight.h"
#include "terminal/term_handler.h"
#include "terminal/windows/edw.h"
#include "terminal/windows/few.h"
#include "terminal/windows/ofw.h"
#include "terminal/windows/pow.h"
#include "utils/clipboard_manager.h"
#include "utils/key_management.h"

#define SHOW_ERROR true

// Global vars.
int color_pair = 5;
int color_index = 20;
cJSON* config;
ParserList parsers;
LSPServerLinkedList lsp_servers;
WorkspaceSettings workspace_settings;

typedef enum {
  EVENT_CONTINUE,
  EVENT_QUIT,
  EVENT_READ_INPUT,
} EventLoopAction;

typedef struct {
  FileContainer* files;
  int file_count;
  int current_file_index;
  ExplorerFolder pwd;
  GUIContext gui_context;
  bool refresh_local_vars;

  WindowHighlightDescriptor highlight_descriptor;
  History* old_history_frame;
  PayloadStateChange payload_state_change;
  Cursor old_selected_cursor;

  MEVENT m_event;
  int peek_c;
  bool mouse_drag;
  time_val last_time_mouse_drag;
  time_val t_date;
  clock_t t_clock;
} EditorContext;

static void setup_environment() {
  if (!SHOW_ERROR) {
    FILE* f = fopen("/dev/null", "w");
    dup2(fileno(f), STDERR_FILENO);
    fclose(f);
  } else {
    FILE* f = fopen("./.logs.txt", "w");
    if (f != NULL) {
      dup2(fileno(f), STDERR_FILENO);
      fclose(f);
    }
  }

  setlocale(LC_ALL, "");
  setlocale(LC_NUMERIC, "C");
  system("echo > .lsp_logs.txt");
}

static void run_post_processing(EditorContext* ctx) {
  FileContainer* fc = &ctx->files[ctx->current_file_index];
  if (ctx->refresh_local_vars == true) {
    gui_closePopup(&ctx->gui_context);
    ctx->refresh_local_vars = false;
    ctx->old_history_frame = fc->history_frame;
    ctx->payload_state_change = getPayloadStateChange(&fc->highlight_data, &fc->lsp_datas);
    ctx->old_selected_cursor = fc->select_cursor;
    gui_updateEDW(&ctx->gui_context);
  }

  // flag cursor change
  if (!cursor_eq(fc->cursor, fc->old_cur)) {
    fc->old_cur = fc->cursor;
    moveScreenToMatchCursor(&ctx->gui_context, fc->cursor, &fc->screen_x, &fc->screen_y);
    gui_updateEDW(&ctx->gui_context);
  }

  // flag selection change
  if (!cursor_eq(fc->select_cursor, ctx->old_selected_cursor)) {
    ctx->old_selected_cursor = fc->select_cursor;
    gui_updateEDW(&ctx->gui_context);
    if (!cursor_is_disabled(fc->select_cursor)) {
      gui_closePopup(&ctx->gui_context);
    }
  }

  // flag screen_x change
  if (fc->old_screen_x != fc->screen_x) {
    gui_updateEDW(&ctx->gui_context);
    gui_adaptPopup(&ctx->gui_context, fc->screen_x - fc->old_screen_x, 0);
    fc->old_screen_x = fc->screen_x;
  }

  // flag screen_y change
  if (fc->old_screen_y != fc->screen_y) {
    gui_updateEDW(&ctx->gui_context);
    gui_adaptPopup(&ctx->gui_context, 0, fc->screen_y - fc->old_screen_y);

    // resize lnw_w to match with line_number_length
    int new_lnw_width = numberOfDigitOfNumber(fc->screen_y + getmaxy(ctx->gui_context.edw_context.ftw));
    if (new_lnw_width != getEDW_LengthLineNumber(&ctx->gui_context)) {
      gui_resizeEDW(&ctx->gui_context, new_lnw_width);
    }
    fc->old_screen_y = fc->screen_y;
  }

  // If it needed to reparse the current file for tree. Looking for state changes.
  if (fc->highlight_data.is_active == true && (ctx->old_history_frame != fc->history_frame || fc->highlight_data.tree == NULL)) {
    parseTree(&fc->root, &fc->history_frame, &fc->highlight_data, &ctx->old_history_frame);
    optimizeHistory(fc->history_root, &fc->history_frame);
    ctx->old_history_frame = fc->history_frame;
  }
}

static void paint_gui(EditorContext* ctx) {
  FileContainer* fc = &ctx->files[ctx->current_file_index];
  // Refresh Editor Windows vars
  if (ctx->gui_context.edw_context.refresh_edw == true) {
    whd_reset(&ctx->highlight_descriptor);

    // calculate tree_sitter Highlight
    TS_highlightCurrentFile(&fc->highlight_data, ctx->gui_context.edw_context.ftw, fc->screen_x, fc->screen_y, fc->cursor,
                            &ctx->highlight_descriptor);

    // add lsp highlights
    LSP_highlightCurrentFile(&fc->lsp_datas, fc->cursor, &ctx->highlight_descriptor, &ctx->gui_context);
  }

  gui_repaintGUI(&ctx->gui_context, &ctx->highlight_descriptor, &ctx->pwd, ctx->files, ctx->file_count, ctx->current_file_index);

  assert(checkFileIntegrity(fc->root) == true);
  assert(checkByteCountIntegrity(fc->root) == true);
}

static void dispatch_lsp(DispatcherPayload* payload, int* c, int* hash) {
  LSPServerLinkedList_Cell* cell = lsp_servers.head;
  while (cell != NULL) {
    while (LSP_dispatchOnReceive(&cell->lsp_server, dispatcher, payload)) {
      if (*c == ERR) {
        *c = ONLY_REPAINT_INPUT;
        *hash = ONLY_REPAINT_INPUT;
      }
    }
    cell = cell->next;
  }
}

static EventLoopAction process_key_event(EditorContext* ctx, int hash, int c) {
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
      handleClick(&ctx->gui_context, &ctx->files, &ctx->file_count, &ctx->current_file_index, &ctx->pwd, cursor, select_cursor, desired_column,
                  screen_x, screen_y, &ctx->refresh_local_vars, &ctx->m_event, &ctx->peek_c, &ctx->mouse_drag, &ctx->last_time_mouse_drag,
                  &ctx->t_date, &ctx->t_clock, &c, &ctx->highlight_descriptor);
      if (!gui_doesGUINeedRepaint(&ctx->gui_context)) {
        return EVENT_READ_INPUT;
      }
      break;

    case H_KEY_RIGHT:
      if (cursor_is_disabled(*select_cursor)) *cursor = moveRight(*cursor);
      setSelectCursorOff(cursor, select_cursor, SELECT_OFF_RIGHT);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      break;
    case H_KEY_LEFT:
      if (cursor_is_disabled(*select_cursor)) *cursor = moveLeft(*cursor);
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
      if (ctx->current_file_index != 0) ctx->current_file_index--;
      ctx->refresh_local_vars = true;
      gui_updateOFW(&ctx->gui_context);
      break;
    case H_KEY_CTRL_MAJ_UP:
      if (ctx->current_file_index != ctx->file_count - 1) ctx->current_file_index++;
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
      if (cursor_is_disabled(*select_cursor)) { *select_cursor = moveLeft(*cursor); }
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, false, false);
      break;
    case H_KEY_SUPPR:
      if (cursor_is_disabled(*select_cursor)) { *select_cursor = moveRight(*cursor); }
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
      } else {
        for (int i = 0; i < TAB_SIZE; i++) {
          *cursor = insertCharInLineC(*cursor, readChar_U8FromInput(' '));
        }
      }
      saveAction(history_frame, createInsertAction(*cursor, tmp), globalOnStageChange, cursor,
                 (long*)&ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      break;
    case CTRL('d'):
      if (cursor_is_disabled(*select_cursor) == true) { selectLine(cursor, select_cursor); }
      deleteSelectionWithState(history_frame, cursor, select_cursor, ctx->payload_state_change);
      setDesiredColumn(*cursor, desired_column);
      gui_closePopup(&ctx->gui_context);
      gui_updateEDW(&ctx->gui_context);
      break;
    case CTRL('e'): switchFEW(&ctx->gui_context); break;
    case CTRL('l'): gui_switchOFW(&ctx->gui_context); break;
    case CTRL(' '): askCompletion(&ctx->gui_context, cursor, screen_x, screen_y, lsp_data, true, true); break;
    case H_KEY_ESCAPE:
    case CTRL('['): gui_closePopup(&ctx->gui_context); break;
    default:
      if (iscntrl(c)) {
        printf("Unsupported touch %d", c);
      } else {
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

int main(int file_count, char** args) {
  setup_environment();

  char** file_names = args;
  file_names++;
  file_count--;

  config = loadConfig();
  initParserList(&parsers);
  initLSPServerList(&lsp_servers);

  EditorContext ctx;
  ctx.file_count = file_count;
  ctx.current_file_index = 0;
  ctx.refresh_local_vars = true;
  ctx.old_history_frame = NULL;
  ctx.mouse_drag = false;
  ctx.last_time_mouse_drag = timeInMilliseconds();
  ctx.t_date = timeInMilliseconds();
  ctx.t_clock = clock();
  ctx.peek_c = -1;

  gui_initGUIContext(&ctx.gui_context);
  gui_initNCurses(&ctx.gui_context);

  setupWorkspace(&workspace_settings, &ctx.file_count, &file_names, &ctx.gui_context, &ctx.current_file_index);
  setupOpenedFiles(&ctx.file_count, file_names, &ctx.files);

  char* dir_path = getenv("PWD");
  if (dir_path == NULL) dir_path = getenv("HOME");
  initFolder(workspace_settings.is_used == true ? workspace_settings.dir_path : dir_path, &ctx.pwd);
  ctx.pwd.open = true;

  whd_init(&ctx.highlight_descriptor);

  while (true) {
    run_post_processing(&ctx);
    paint_gui(&ctx);

  read_input:;
    int c;
    if (ctx.peek_c == -1) {
      c = getch();
    } else {
      c = ctx.peek_c;
      ctx.peek_c = -1;
    }
    int hash = c;

    ctx.t_date = timeInMilliseconds();
    ctx.t_clock = clock();

    if (c != KEY_MOUSE && c != -1) {
      fprintf(stderr, "Code %d, Key : '%s' hash into %d.\n", c, keyname(c), hashString((unsigned char*)keyname(c)));
      const char* key_str = keyname(c);
      if (key_str != NULL && key_str[0] != '\0') {
        if (key_str[0] != '^') {
          hash = hashString((unsigned char*)key_str);
        }
      } else {
        fprintf(stderr, "keyname is NULL");
      }
    }

    DispatcherPayload payload;
    payload.files_state = filesStateOf(&ctx.files, &ctx.file_count, &ctx.current_file_index, &ctx.refresh_local_vars);
    payload.view_port = viewPortOf(&ctx.gui_context, &ctx.files[ctx.current_file_index].screen_x, &ctx.files[ctx.current_file_index].screen_y);
    payload.cursor = &ctx.files[ctx.current_file_index].cursor;

    dispatch_lsp(&payload, &c, &hash);

    if (ctx.refresh_local_vars) {
      continue;
    }

    if (hash == ERR) {
      goto read_input;
    }

    if (hash == KEY_MOUSE) {
      if (getmouse(&ctx.m_event) != OK) {
        fprintf(stderr, "MOUVE_EVENT_NOT_OK !\r\n");
        goto read_input;
      }
      detectComplexMouseEvents(&ctx.m_event);
    }

    bool has_popup_handle_input = gui_handlePopupInput(
        &ctx.gui_context, &ctx.files[ctx.current_file_index].cursor, hash, c,
        ctx.files[ctx.current_file_index].lsp_datas.computed,
        &ctx.files[ctx.current_file_index].history_frame, ctx.payload_state_change, &payload,
        (hash == KEY_MOUSE ? &ctx.m_event : NULL));

    if (has_popup_handle_input) {
      c = ONLY_REPAINT_INPUT;
      hash = ONLY_REPAINT_INPUT;
    }

    EventLoopAction action = process_key_event(&ctx, hash, c);

    if (action == EVENT_QUIT) goto end;
    if (action == EVENT_READ_INPUT) goto read_input;
  }

end:
  printf("\033[?1003l\033[0 q\n");
  fflush(stdout);

  whd_free(&ctx.highlight_descriptor);

  if (workspace_settings.is_used == true) {
    WorkspaceSettings new_settings;
    getWorkspaceSettingsForCurrentDir(&new_settings, ctx.files, ctx.file_count, ctx.current_file_index,
                                      ctx.gui_context.ofw_context.ofw_height != 0, ctx.gui_context.few_context.few_width != 0,
                                      FILE_EXPLORER_WIDTH);
    saveWorkspaceSettings(workspace_settings.dir_path, &new_settings);
    destroyWorkspaceSettings(&new_settings);
  }

  for (int i = 0; i < ctx.file_count; i++) {
    destroyFileContainer(ctx.files + i);
  }
  destroyFolder(&ctx.pwd);
  free(ctx.files);
  cJSON_Delete(config);
  destroyParserList(&parsers);

  usleep(30000);
  flushinp();
  endwin();
  return 0;
}
