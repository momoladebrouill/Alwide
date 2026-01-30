#include "lsp_completion.h"

#include <assert.h>
#include <string.h>

#include "../../../io_management/viewport_history.h"
#include "../../../terminal/windows/edw.h"
#include "../../../terminal/windows/pow.h"
#include "../../../utils/global-variables.h"


void applyTextEdit(Cursor* cursor, TextEdit* text_edit, History** history_p, PayloadStateChange payload_state_change) {
  // As a text edit can represent a "replacement" we have to handle this deleting old text and inserting new text after.
  // Delete part
  *cursor = tryToReachAbsPosition(*cursor, text_edit->range.pos1.row + 1, text_edit->range.pos1.column);
  Cursor end = tryToReachAbsPosition(*cursor, text_edit->range.pos2.row + 1, text_edit->range.pos2.column);
  deleteSelectionWithState(history_p, cursor, &end, payload_state_change);
  // insert part
  *cursor = insertCharArrayAtCursorWithHist(history_p, *cursor, text_edit->new_text, payload_state_change);
}

int compareTextEdit(const void* e1_p, const void* e2_p) {
  TextEdit* e1 = (TextEdit*)e1_p;
  TextEdit* e2 = (TextEdit*)e2_p;
  if (e1->range.pos1.row < e2->range.pos1.row)
    return 1;
  if (e1->range.pos1.row > e2->range.pos1.row)
    return -1;

  return e1->range.pos1.column <= e2->range.pos1.column ? 1 : -1;
}

Range getReplaceRange(Cursor* cursor, char insertText[METHOD_MAX_LENGTH]) {
  Cursor select = disableCursor(*cursor);
  if (cursor->line_id.absolute_column != 0) {
    Cursor tmp = *cursor;
    selectWord(&tmp, &select);
  }

  Cursor begin = select;
  int index = 0;

  bool begin_matching = true;
  while (!isCursorDisabled(select) && isCursorStrictPreviousThanOther(select, *cursor)) {
    select = moveRight(select);
    Char_U8 ch = getCharAtCursor(select);
    for (int i = 0; i < sizeChar_U8(ch); i++) {
      assert(index <= METHOD_MAX_LENGTH);
      if (insertText[index] != ch.t[i] && towupper(insertText[index]) != towupper(ch.t[i])) {
        begin_matching = false;
        break;
      }
      index++;
    }
  }

  if (isCursorDisabled(select) || begin_matching == false) {
    begin = *cursor;
  }

  return (Range){.pos1 = {.row = begin.file_id.absolute_row - 1, .column = begin.line_id.absolute_column},
                 .pos2 = {.row = cursor->file_id.absolute_row - 1, .column = cursor->line_id.absolute_column}};
}

void LSP_executeCompletion(Cursor* cursor, CompletionItem* item, History** history_p,
                           PayloadStateChange payload_state_change) {
  if (!item->is_text_edit) {
    // copy the text to the edit.
    item->text_edit.new_text = malloc(sizeof(char) * METHOD_MAX_LENGTH);
    memcpy(item->text_edit.new_text, item->insertText, sizeof(char) * METHOD_MAX_LENGTH);

    // we have to detect what to replace.
    item->text_edit.range = getReplaceRange(cursor, item->insertText);
    item->is_text_edit = true;
  }

  bool main_text_edit_done = false;
  CursorDescriptor position_after_insert;
  position_after_insert.row = -1;
  position_after_insert.column = -1;

  // TODO may check if it check well using multiple additionalTextEdit
  qsort(item->additionalTextEdits, item->additionalTextEditsSize, sizeof(TextEdit), compareTextEdit);

  int i = 0;
  while (i < item->additionalTextEditsSize) {
    if (!main_text_edit_done && compareTextEdit(&item->text_edit, item->additionalTextEdits + i) != 1) {
      main_text_edit_done = true;
      applyTextEdit(cursor, &item->text_edit, history_p, payload_state_change);
      position_after_insert = cursorToDescriptor(cursor);
    }
    else {
      *cursor = tryToReachAbsPosition(*cursor, item->additionalTextEdits[i].range.pos1.row + 1,
                                      item->additionalTextEdits[i].range.pos1.column);
      CursorDescriptor tmp = cursorToDescriptor(cursor);
      applyTextEdit(cursor, item->additionalTextEdits + i, history_p, payload_state_change);
      i++;
      // if the end position of the cursor is already setted we have to follow the new add in the file to follow lines.
      // !! ISSUE !! we don't manage the follow if some column are added before in the same line.
      // We don't manage if the additionnal text edit is removing a lot of line.
      if (position_after_insert.row != -1) {
        position_after_insert.row += cursor->file_id.absolute_row - tmp.row;
      }
    }
  }

  if (!main_text_edit_done) {
    applyTextEdit(cursor, &item->text_edit, history_p, payload_state_change);
  }
  else if (position_after_insert.row != -1) {
    *cursor = tryToReachAbsPosition(*cursor, position_after_insert.row, position_after_insert.column);
  }
}


void askCompletion(GUIContext* gui_context, Cursor* cursor, int* screen_x, int* screen_y, LSP_Data* lsp_data,
                   bool reset, bool force) {
  if (lsp_data->is_enable) {
    if (!force && !isAfterAWord(cursor)) {
      LSP_destroyCompletionList(&lsp_data->computed->completions);
      gui_closePopup(gui_context);
      return;
    }
    if (reset) {
      LSP_destroyCompletionList(&lsp_data->computed->completions);
    }
    LSP_requestCompletion(getLSPServerForLanguage(&lsp_servers, lsp_data->lang_id), lsp_data->path_abs,
                          getAbsRow(cursor), getAbsCol(cursor));
    if (gui_context->edw_context.pow_owner != COMPLETION) {
      gui_setLastTextAnchor(gui_context, cursorToDescriptor(cursor));
    }
    else {
      ViewPort view_port = getViewPort(gui_context, screen_x, screen_y);
      gui_showCompletionTextAnchor(&view_port, cursor);
    }
  }
}
