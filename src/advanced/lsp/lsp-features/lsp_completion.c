#include "lsp_completion.h"

#include <assert.h>
#include <string.h>

#include "../../../environnement/global-variables.h"
#include "../../../io-management/viewport_history.h"
#include "../../../terminal/windows/edw.h"
#include "../../../terminal/windows/pow.h"
#include "lsp_signature_help.h"
#include "lsp_tools.h"


LSP_Range getReplaceRange(Cursor* cursor, char insertText[METHOD_MAX_LENGTH]) {
  Cursor select = cursor_disable(*cursor);
  if (cursor->line_id.absolute_column != 0) {
    Cursor tmp = *cursor;
    selectWord(&tmp, &select);
  }

  Cursor begin = select;
  int index = 0;

  bool begin_matching = true;
  while (!cursor_is_disabled(select) && cursor_lt(select, *cursor)) {
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

  if (cursor_is_disabled(select) || begin_matching == false) {
    begin = *cursor;
  }

  return LSP_range_from_cursor(begin.file_id.absolute_row, begin.line_id.absolute_column, cursor->file_id.absolute_row,
                               cursor->line_id.absolute_column);
}

void executeLSPCompletion(Cursor* cursor, LSP_CompletionItem* item, History** history_p,
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
  qsort(item->additionalTextEdits, item->additionalTextEditsSize, sizeof(LSP_TextEdit), compareTextEdit);

  int i = 0;
  while (i < item->additionalTextEditsSize) {
    if (!main_text_edit_done && compareTextEdit(&item->text_edit, item->additionalTextEdits + i) != 1) {
      main_text_edit_done = true;
      applyTextEdit(cursor, &item->text_edit, history_p, payload_state_change);
      position_after_insert = cursor_to_desc(*cursor);
    }
    else {
      *cursor = tryToReachAbsPosition(*cursor, item->additionalTextEdits[i].range.pos1.row + 1,
                                      item->additionalTextEdits[i].range.pos1.column);
      CursorDescriptor tmp = cursor_to_desc(*cursor);
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


void askCompletion(GUIContext* gui_context, FileContainer* fc, bool reset, bool force) {
  if (fc->lsp_datas.is_enable) {
    if (hasElementBeforeLine(fc->cursor.line_id)) {
      Char_U8 u8 = getCharAtCursor(fc->cursor);
      if (areChar_U8Equals(u8, readChar_U8FromCharArray("("))) {
        askSignatureHelp(fc, &fc->cursor);
        return;
      }
    }
    if (!force && gui_context->edw_context.pow_owner == SIGNATURE_HELP) {
      return;
    }
    if (!force && !isAfterAWord(&fc->cursor)) {
      LSP_destroyCompletionList(&fc->lsp_datas.computed->completions);
      gui_closePopup(gui_context);
      return;
    }
    if (reset) {
      LSP_destroyCompletionList(&fc->lsp_datas.computed->completions);
    }
    LSP_requestCompletion(getLSPServerForLanguage(&lsp_servers, fc->lsp_datas.lang_id), fc->lsp_datas.path_abs,
                          LSP_pos_from_cursor(cursor_row(fc->cursor), cursor_col(fc->cursor)));
    if (gui_context->edw_context.pow_owner != COMPLETION) {
      gui_setLastTextAnchor(gui_context, cursor_to_desc(fc->cursor));
    }
    else {
      ViewPort view_port = viewPortOf(gui_context, &fc->screen_x, &fc->screen_y);
      gui_showGenericPopupWithTextAnchor(&view_port, &fc->cursor, 7, 50, COMPLETION);
    }
  }
}

void receiveCompletionData(cJSON* packet, FileContainer* file, ViewPort* view_port, Cursor* cursor) {
  LSP_destroyCompletionList(&file->lsp_datas.computed->completions);
  LSP_getCompletionListFromJSON(LSP_getPacketResult(packet), &file->lsp_datas.computed->completions);

  // if there is no hover data we close the popup
  if (file->lsp_datas.computed->completions.completions.size == 0) {
    if (view_port->gui->edw_context.pow_owner == COMPLETION) {
      gui_closePopup(view_port->gui);
    }
    return;
  }

  gui_resumeCompletionTextAnchor(view_port, cursor);
}
