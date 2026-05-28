#include "lsp_emitter.h"

#include "../../environnement/global_variables.h"
#include "../../utils/tools.h"
#include "../../utils/key_management.h"

int global_version = 2;

static int getLSPColumnFromCursor(Cursor cursor, int row, int character_column) {
  Cursor target = tryToReachAbsPosition(cursor, row + 1, 0);
  LineNode* line = target.line_id.line;
  return getUTF16Offset(line->ch, line->element_number, character_column);
}

void onStateChangeLSP(Action action, LSP_Data* data, Cursor* cursor) {
  if (!data->is_enable) {
    return;
  }
  LSP_Server* lsp_server = getLSPServerForLanguage(&lsp_servers, data->lang_id);
  if (lsp_server == NULL) {
    return;
  }

  ChangeDescriptor action_change = actionToChangeDescriptor(action);

  int start_lsp_col = getLSPColumnFromCursor(*cursor, action_change.start_point.row, action_change.start_point.column);
  int old_end_lsp_col = start_lsp_col;
  // int new_end_lsp_col = start_lsp_col; // Currently not used directly for range

  if (action.action == INSERT) {
    // new_end_lsp_col = getLSPColumnFromCursor(*cursor, action_change.new_end_point.row, action_change.new_end_point.column);
    old_end_lsp_col = start_lsp_col;
  }
  else if (action.action == DELETE || action.action == DELETE_ONE) {
    if (action.action == DELETE) {
      // For multi-line DELETE, we need to find the UTF-16 offset on the LAST line of the deletion.
      int rows_deleted = 0;
      int last_line_utf16_len = 0;
      int index = 0;
      int length = action.byte_end - action.byte_start;
      while (index < length && action.ch[index] != '\0') {
        if (action.ch[index] == '\n') {
          rows_deleted++;
          last_line_utf16_len = 0;
          index++;
        }
        else {
          Char_U8 u8 = readChar_U8FromCharArrayWithFirst((char*)action.ch + index, action.ch[index]);
          index += sizeChar_U8(u8);
          last_line_utf16_len += getUTF16Length(u8);
        }
      }

      if (rows_deleted > 0) {
        old_end_lsp_col = last_line_utf16_len;
      }
      else {
        old_end_lsp_col = start_lsp_col + last_line_utf16_len;
      }
    }
    else {
      Char_U8 u8 = readChar_U8FromInput(K_CODE(action.unique_ch));
      if (action.unique_ch == '\n') {
        old_end_lsp_col = 0;
      }
      else {
        old_end_lsp_col = start_lsp_col + getUTF16Length(u8);
      }
    }
  }

  // Create array
  cJSON* array_of_changes = cJSON_CreateArray();

  // Create the object that the array will contain, describe de action_change
  cJSON* change = cJSON_CreateObject();
  // Add the 'range' field to the change
  LSP_Range lsp_range = LSP_range(LSP_pos(action_change.start_point.row, start_lsp_col),
                                  LSP_pos(action_change.old_end_point.row, old_end_lsp_col));
  cJSON* range = LSP_getJSONRange(lsp_range);
  cJSON_AddItemToObject(change, "range", range);
  // Add the 'text' field to the change
  Cursor begin =
    tryToReachAbsPosition(*cursor, LSP_0_row_to_1_row(action_change.start_point.row), action_change.start_point.column);
  Cursor end = tryToReachAbsPosition(*cursor, LSP_0_row_to_1_row(action_change.new_end_point.row),
                                     action_change.new_end_point.column);
  char* new_text = dumpSelection(begin, end);
  cJSON_AddStringToObject(change, "text", new_text);
  free(new_text);

  // Add the change to the array
  cJSON_AddItemToArray(array_of_changes, change);

  // Send the changes to the lsp server
  LSP_notifyLspFileDidChange(lsp_server, data->path_abs, array_of_changes, global_version++);
}
