#include "lsp_hover.h"

#include <assert.h>

#include "../../../terminal/term_handler.h"
#include "../../../terminal/windows/edw.h"
#include "../../../terminal/windows/pow.h"
#include "../lsp-features/lsp_completion.h"


void receiveHoverData(cJSON* packet, FileContainer* file, ViewPort* view_port, Cursor* cursor, void* payload) {
  LSP_Hover hover;
  LSP_getHoverFromJSON(LSP_getPacketResult(packet), &hover);
  assert(payload != NULL);

  // if there is no hover data we don't override the current data and we close the popup.
  if (hover.size == 0) {
    gui_closePopup(view_port->gui);
    free(payload);
    return;
  }
  LSP_destroyHover(&file->lsp_datas.computed->hover);
  file->lsp_datas.computed->hover = hover;

  // create range using cursor position when request if not present
  if (file->lsp_datas.computed->hover.size > 0 && file->lsp_datas.computed->hover.is_range == false) {
    LSP_Position* pos = payload;

    Cursor end = tryToReachAbsPosition(*cursor, LSP_0_row_to_1_row(pos->row), pos->column);
    Cursor begin = cursor_disable(*cursor);
    selectWord(&end, &begin);

    // set up the new range
    file->lsp_datas.computed->hover.range.pos1 =
      LSP_pos_from_cursor(begin.file_id.absolute_row, begin.line_id.absolute_column);
    file->lsp_datas.computed->hover.range.pos2 = LSP_pos_from_cursor(end.file_id.absolute_row, end.line_id.absolute_column);
    file->lsp_datas.computed->hover.is_range = true;
  }

  // We assert that every data going outfrom there has a range.
  assert(file->lsp_datas.computed->hover.is_range);
  gui_resumeHoverInformation(cursor, view_port, &file->lsp_datas.computed->hover, ft_tab_size(file->feature));

  free(payload);
}
