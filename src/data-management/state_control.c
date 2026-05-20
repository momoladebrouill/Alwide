#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "file_management.h"
#include "state_control.h"


void initHistory(History* history) {
  history->action.action = ACTION_NONE;
  history->action.time = 0;
  history->prev = NULL;
  history->next = NULL;
}

Cursor undo(History** history_p, Cursor cursor, void (*onEachStateChange)(Action action, Cursor* cursor, void* payload),
            void* payload, ft_Tabulation* tab) {
  History* history = *history_p;

  // If history is at root return and do nothing. Can't undo ;).
  if (history->action.action == ACTION_NONE) {
    return cursor;
  }

  time_val canceled_time_action = history->action.time;
  cursor = doReverseAction(&history->action, cursor, onEachStateChange, payload, tab);
  history->action.time = canceled_time_action;

  *history_p = history->prev;

  if (diff2Time(history->action.time, history->prev->action.time) < TIME_CONSIDER_UNIQUE_UNDO) {
    return undo(history_p, cursor, onEachStateChange, payload, tab);
  }

  return cursor;
}

Cursor redo(History** history_p, Cursor cursor, void (*onEachStateChange)(Action action, Cursor* cursor, void* payload),
            void* payload, ft_Tabulation* tab) {
  History* history = *history_p;

  // If history is at the end return and do nothing. Cannot redo nothing ;).
  if (history->next == NULL) {
    return cursor;
  }

  *history_p = history->next;
  history = *history_p;

  time_val canceled_time_action = history->action.time;

  cursor = doReverseAction(&history->action, cursor, onEachStateChange, payload, tab);
  history->action.time = canceled_time_action;

  if (history->next != NULL &&
      diff2Time(history->action.time, history->next->action.time) < TIME_CONSIDER_UNIQUE_UNDO) {
    return redo(history_p, cursor, onEachStateChange, payload, tab);
  }

  return cursor;
}


void saveAction(History** history_p, Action action,
                void (*onEachStateChange)(Action action, Cursor* cursor, void* payload), Cursor* cursor,
                void* payload) {
  History* history = *history_p;
  if (action.action == ACTION_NONE) {
    return;
  }

  destroyEndOfHistory(history);
  assert(history->next == NULL);

  assert(history != NULL);
  history->next = malloc(sizeof(History));

  assert(history->next != NULL);
  history->next->prev = history;
  history->next->next = NULL;
  history->next->action = action;

  history = history->next;

  *history_p = history;


  if (onEachStateChange != NULL) {
    onEachStateChange(action, cursor, payload);
  }
}

Cursor doReverseAction(Action* action_p, Cursor cursor,
                       void (*onEachStateChange)(Action action, Cursor* cursor, void* payload), void* payload,
                       ft_Tabulation* tabulation) {
  Action action = *action_p;
  Cursor tmp;
  Cursor tmp_end;
  ACTION_TYPE type = action.action;
  switch (type) {
    case DELETE_ONE:
      tmp.file_id = tryToReachAbsRow(cursor.file_id, action.cur.row);
      tmp.line_id = moduloLineIdentifierR(getLineForFileIdentifier(tmp.file_id), action.cur.column);
      if (action.unique_ch == '\n') {
        cursor = insertNewLineInLineC(tmp);
        destroyAction(action);
        *action_p = createInsertAction(tmp, cursor_to_desc(cursor));
        if (onEachStateChange != NULL) {
          onEachStateChange(*action_p, &cursor, payload);
        }
        return cursor;
      }
      cursor = insertCharInLineC(tmp, readChar_U8FromInput(action.unique_ch));
      destroyAction(action);
      *action_p = createInsertAction(tmp, cursor_to_desc(cursor));
      if (onEachStateChange != NULL) {
        onEachStateChange(*action_p, &cursor, payload);
      }
      return cursor;
    case DELETE:
      tmp.file_id = tryToReachAbsRow(cursor.file_id, action.cur.row);
      tmp.line_id = moduloLineIdentifierR(getLineForFileIdentifier(tmp.file_id), action.cur.column);
      cursor = insertCharArrayAtCursor(tmp, action.ch, tabulation);
      destroyAction(action);
      *action_p = createInsertAction(tmp, cursor_to_desc(cursor));
      if (onEachStateChange != NULL) {
        onEachStateChange(*action_p, &cursor, payload);
      }
      return cursor;
    case INSERT:
      tmp.file_id = tryToReachAbsRow(cursor.file_id, action.cur.row);
      tmp.line_id = moduloLineIdentifierR(getLineForFileIdentifier(tmp.file_id), action.cur.column);
      tmp_end.file_id = tryToReachAbsRow(cursor.file_id, action.cur_end.row);
      tmp_end.line_id = moduloLineIdentifierR(getLineForFileIdentifier(tmp_end.file_id), action.cur_end.column);

      destroyAction(action);
      *action_p = createDeleteAction(tmp, cursor_to_desc(tmp_end));
      deleteSelection(&tmp, &tmp_end);
      if (onEachStateChange != NULL) {
        onEachStateChange(*action_p, &tmp, payload);
      }
      return tmp;
    case ACTION_NONE:
      return cursor;
    default:
      assert(false);
  }
}

Action createDeleteAction(Cursor cur1, CursorDescriptor cur2_desc) {
  if (cursor_is_disabled(cur1) || cursor_desc_is_disabled(cur2_desc)) {
    Action action;
    action.action = ACTION_NONE;
    action.time = 0;
    return action;
  }

  Cursor cur2 = tryToReachAbsPosition(cur1, cur2_desc.row, cur2_desc.column);

  if (cursor_le(cur2, cur1)) {
    Cursor tmp = cur1;
    cur1 = cur2;
    cur2 = tmp;
  }

  Action action;
  action.time = timeInMilliseconds();
  action.cur = cursor_to_desc(cur1);
  // Not used.
  Cursor disabled_cursor = cursor_disable(cur1);
  action.cur_end = cursor_to_desc(disabled_cursor);

  char* dump = dumpSelection(cur1, cur2);

  if (dump[0] == '\0') { // Empty selection
    free(dump);
    action.action = ACTION_NONE;
    action.time = 0;
    return action;
  }

  // If there is only 1 element to save we don't use malloc. We use unique_ch.
  if (dump[1] == '\0') {
    action.action = DELETE_ONE;
    action.ch = NULL;
    action.unique_ch = dump[0];
    action.byte_start = getIndexForCursor(cur1);
    action.byte_end = action.byte_start + 1;

    free(dump);
    return action;
  }


  action.action = DELETE;
  action.unique_ch = '\0';
  action.ch = dump;
  action.byte_start = getIndexForCursor(cur1);
  action.byte_end = getIndexForCursor(cur2);

  assert(action.ch != NULL);

  return action;
}

Action createInsertAction(Cursor cur1, CursorDescriptor cur2_desc) {
  Cursor cur2 = tryToReachAbsPosition(cur1, cur2_desc.row, cur2_desc.column);
  Action action;
  action.action = INSERT;
  // Know that cur is the first cursor can be useful.
  if (cursor_le(cur2, cur1) == true) {
    Cursor tmp = cur1;
    cur1 = cur2;
    cur2 = tmp;
  }
  action.time = timeInMilliseconds();
  action.unique_ch = '\0';
  action.ch = NULL;
  action.cur = cursor_to_desc(cur1);
  action.cur_end = cursor_to_desc(cur2);
  action.byte_start = getIndexForCursor(cur1);
  action.byte_end = getIndexForCursor(cur2);
  return action;
}

void destroyAction(Action action) {
  assert(action.action != ACTION_NONE);
  free(action.ch);
}

void destroyEndOfHistory(History* history) {
  assert(history != NULL);
  History* tmp = history;

  history = history->next;
  while (history != NULL) {
    History* tmp2 = history;
    history = history->next;

    assert(tmp2->action.action != ACTION_NONE);
    destroyAction(tmp2->action);
    free(tmp2);
  }

  tmp->next = NULL;
}


void createTmpDir() {
  char command[PATH_MAX];
  snprintf(command, sizeof(command), "mkdir -p \"%s\"", FILE_STATE_PATH);
  system(command);
}

void saveCurrentStateControl(History root, History* current_state, char* fileName) {
  createTmpDir();

  char fileStateControl[PATH_MAX];
  snprintf(fileStateControl, sizeof(fileStateControl), "%s%llu", FILE_STATE_PATH, hashFileName(fileName));


  FILE* f = fopen(fileStateControl, "w");
  if (f == NULL) {
    printf("Impossible to save state control, couldn't open file %s.\r\n", fileStateControl);
    return;
  }

  struct stat attr;
  stat(fileName, &attr);
  fprintf(f, "%ld\n", (long)attr.st_mtime);

  History* current = root.next;
  while (current != NULL) {
    char isCurrentState = current == current_state ? 'y' : 'n';
    fprintf(f, "%c %c %lld\n", isCurrentState, current->action.action, current->action.time);

    switch (current->action.action) {
      case DELETE:
        fprintf(f, "%d %d %d\n", current->action.cur.row, current->action.cur.column, current->action.byte_start);
        fprintf(f, "%ld\n", strlen(current->action.ch));
        fprintf(f, "%s\n", current->action.ch);
        break;
      case DELETE_ONE:
        fprintf(f, "%d %d %d\n", current->action.cur.row, current->action.cur.column, current->action.byte_start);
        fprintf(f, "%c\n", current->action.unique_ch);
        break;
      case INSERT:
        fprintf(f, "%d %d %d\n", current->action.cur.row, current->action.cur.column, current->action.byte_start);
        fprintf(f, "%d %d %d\n", current->action.cur_end.row, current->action.cur_end.column, current->action.byte_end);
        break;
      case ACTION_NONE:
        break;
      default:
        assert(false);
    }
    current = current->next;
  }


  fclose(f);
}

void loadCurrentStateControl(History* root, History** current_state, IO_FileID io_file) {
  *current_state = root;
  initHistory(root);

  if (io_file.status != EXIST) {
    return;
  }

  char fileStateControl[PATH_MAX];
  snprintf(fileStateControl, sizeof(fileStateControl), "%s%llu", FILE_STATE_PATH, hashFileName(io_file.path_abs));

  FILE* f = fopen(fileStateControl, "r");
  if (f == NULL) {
    return;
  }

  struct stat attr;
  stat(io_file.path_abs, &attr);

  long loaded_sec;
  if (fscanf(f, "%ld\n", &loaded_sec) != 1) {
    fclose(f);
    return;
  }

  if (loaded_sec != (long)attr.st_mtime) {
    fclose(f);
    return;
  }

  // TODO may use fread() instead of fscanf().
  History* state_to_return = *current_state;
  char scan_separtor;
  while (true) {
    char isCurrentState;
    if (fscanf(f, "%c", &isCurrentState) == EOF) {
      break;
    }

    Action action;
    action.ch = NULL;
    char tmp_ch_action_read;
    fscanf(f, "%c%c%c%lld%c", &scan_separtor, &tmp_ch_action_read, &scan_separtor, &action.time, &scan_separtor);
    action.action = tmp_ch_action_read;

    switch (action.action) {
      case DELETE:
        fscanf(f, "%d%c%d%c%d%c", &action.cur.row, &scan_separtor, &action.cur.column, &scan_separtor,
               &action.byte_start, &scan_separtor);
        long str_len;
        fscanf(f, "%ld%c", &str_len, &scan_separtor);
        action.ch = malloc(str_len + 1);
        assert(action.ch != NULL);
        for (int i = 0; i < str_len; i++) {
          fscanf(f, "%c", action.ch + i);
        }
        action.ch[str_len] = '\0';
        // printf("%ld\r\n", strlen(action.ch));
        // printf("%s\r\n", action.ch);
        // exit(0);
        fscanf(f, "%c", &scan_separtor);
        break;
      case DELETE_ONE:
        fscanf(f, "%d%c%d%c%d%c", &action.cur.row, &scan_separtor, &action.cur.column, &scan_separtor,
               &action.byte_start, &scan_separtor);
        fscanf(f, "%c%c", &action.unique_ch, &scan_separtor);
        break;
      case INSERT:
        fscanf(f, "%d%c%d%c%d%c", &action.cur.row, &scan_separtor, &action.cur.column, &scan_separtor,
               &action.byte_start, &scan_separtor);
        fscanf(f, "%d%c%d%c%d%c", &action.cur_end.row, &scan_separtor, &action.cur_end.column, &scan_separtor,
               &action.byte_end, &scan_separtor);
        break;
      case ACTION_NONE:
        break;
      default:
        printf("Current state : %c\r\n", isCurrentState);
        printf("HERE %c\r\n", action.action);
        assert(false);
    }

    // Do not add a onEachStateChange function, here change must not be flagged ! Loading history.
    saveAction(current_state, action, NULL, NULL, NULL);
    if (isCurrentState == 'y') {
      state_to_return = *current_state;
    }
  }

  *current_state = state_to_return;
}


/*
 * Replace multiple delete or multiple insert by one big insert and one big delete.
 */
void optimizeHistory(History* root, History** history_frame) {
  History* current = root;

  while (current != NULL && current->next != NULL) {
    History* next = current->next;
    if (diff2Time(current->action.time, next->action.time) < TIME_CONSIDER_UNIQUE_UNDO && current != *history_frame) {
      bool is_pos_linked = false;
      switch (current->action.action) {
        case DELETE:
        case DELETE_ONE:
          switch (next->action.action) {
            case DELETE_ONE:
              if (next->action.unique_ch == '\n') {
                if (current->action.cur.column == 0 && next->action.cur.column == current->action.cur.row - 1) {
                  // We may don't want to join when line is removed.
                  is_pos_linked = false;
                }
              }
              else {
                if (current->action.cur.row == next->action.cur.row &&
                    current->action.cur.column - 1 == next->action.cur.column) {
                  is_pos_linked = true;
                }
              }

              if (is_pos_linked) {
                if (next == *history_frame) {
                  *history_frame = current;
                }

                if (current->action.action == DELETE_ONE) {
                  current->action.action = DELETE;
                  current->action.ch = malloc(3 * sizeof(char));
                  current->action.ch[0] = next->action.unique_ch;
                  current->action.ch[1] = current->action.unique_ch;
                  current->action.ch[2] = '\0';
                }
                else {
                  int old_size = strlen(current->action.ch);
                  current->action.ch =
                    realloc(current->action.ch, (old_size + 2 /*new char and null char*/) * sizeof(char));
                  memmove(current->action.ch + 1, current->action.ch, old_size);
                  current->action.ch[0] = next->action.unique_ch;
                  current->action.ch[old_size + 1] = '\0';
                }

                current->action.cur = next->action.cur;
                current->action.time = next->action.time;
                current->next = next->next;
                destroyAction(next->action);
                free(next);
              }
              break;
            default:
              break;
          }
          break;

        case INSERT:
          switch (next->action.action) {
            case INSERT:
              if (next->action.unique_ch == '\n') {
                // TODO change condition
                // if (history->action.cur.column == 0 && action.cur.column ==
                // history->action.cur.row - 1) { We may don't want to join when line is removed.
                // is_pos_linked = false;
                // }
              }
              else {
                if (current->action.cur_end.row == next->action.cur.row &&
                    current->action.cur_end.column == next->action.cur.column) {
                  is_pos_linked = true;
                }
              }

              if (is_pos_linked) {
                if (next == *history_frame) {
                  *history_frame = current;
                }
                current->action.cur_end = next->action.cur_end;
                current->action.time = next->action.time;
                current->next = next->next;
                destroyAction(next->action);
                free(next);
                return;
              }
              break;

            default:
              break;
          }
          break;

        default:
          break;
      }
    }
    current = current->next;
  }
}


ChangeDescriptor actionToChangeDescriptor(Action action) {
  ChangeDescriptor edit_local;
  ChangeDescriptor* edit = &edit_local;
  switch (action.action) {
    case INSERT:
      // system("echo \"=== INSERT ===\" >> tree_logs.txt");
      assert(action.byte_start != -1);
      assert(action.byte_end != -1);
      edit->start_byte = action.byte_start;
      edit->start_point.row = action.cur.row - 1;
      edit->start_point.column = action.cur.column;

      edit->old_end_byte = action.byte_start;
      edit->old_end_point.row = edit->start_point.row;
      edit->old_end_point.column = edit->start_point.column;

      edit->new_end_byte = action.byte_end;
      edit->new_end_point.row = action.cur_end.row - 1;
      edit->new_end_point.column = action.cur_end.column;
      // To force the match with previous node.
      break;
    case DELETE:
      // system("echo \"=== DELETE ===\" >> tree_logs.txt");
      assert(action.byte_start != -1);
      edit->start_byte = action.byte_start;
      edit->start_point.row = action.cur.row - 1;
      edit->start_point.column = action.cur.column;

      edit->old_end_byte = action.byte_end;

      // TODO may optimize
      // CALCULATE ROW AND COLUMN POINT
      char* ch = action.ch;
      int current_row = edit->start_point.row;
      int current_column = edit->start_point.column;

      countStringFrame(ch, action.byte_end - action.byte_start, &current_row, &current_column, NULL, 2);

      edit->old_end_point.row = current_row;
      edit->old_end_point.column = current_column;


      edit->new_end_byte = action.byte_start;
      edit->new_end_point.row = edit->start_point.row;
      edit->new_end_point.column = edit->start_point.column;
      // To force the match with previous node.
      break;
    case DELETE_ONE:
      // system("echo \"=== DELETE_ONE ===\" >> tree_logs.txt");
      assert(action.byte_start != -1);
      edit->start_byte = action.byte_start;
      edit->start_point.row = action.cur.row - 1;
      edit->start_point.column = action.cur.column;

      edit->old_end_byte = action.byte_start + 1;
      edit->old_end_point.row = edit->start_point.row;
      edit->old_end_point.column = edit->start_point.column;
      if (action.unique_ch == '\n') {
        edit->old_end_point.row++;
        edit->old_end_point.column = 0;
      }
      else {
        edit->old_end_point.column++;
      }

      edit->new_end_byte = action.byte_start;
      edit->new_end_point.row = edit->start_point.row;
      edit->new_end_point.column = edit->start_point.column;
      // To force the match with previous node.

      break;
    default:
      assert(action.action == ACTION_NONE);
  }
  return edit_local;
}
