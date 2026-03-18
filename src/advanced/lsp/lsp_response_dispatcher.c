#include "lsp_response_dispatcher.h"

#include <assert.h>
#include <string.h>

#include "../../terminal/term_handler.h"
#include "../../terminal/windows/edw.h"
#include "../../terminal/windows/pow.h"

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

void receiveHoverData(cJSON* packet, FileContainer* file, ViewPort* view_port, Cursor* cursor, void* payload) {
  Hover hover;
  LSP_getHoverFromJSON(LSP_getPacketResult(packet), &hover);
  assert(payload != NULL);

  // if there is no hover data we don't override the current data and we close the popup.
  if (hover.size == 0) {
    gui_closePopup(view_port->gui);
    return;
  }
  LSP_destroyHover(&file->lsp_datas.computed->hover);
  file->lsp_datas.computed->hover = hover;

  // create range using cursor position when request if not present
  if (file->lsp_datas.computed->hover.size > 0 && file->lsp_datas.computed->hover.is_range == false) {
    Position* pos = payload;

    Cursor end = tryToReachAbsPosition(*cursor, pos->row, pos->column);
    Cursor begin = disableCursor(*cursor);
    selectWord(&end, &begin);

    // set up the new range
    file->lsp_datas.computed->hover.range.pos1 =
      (Position){.row = begin.file_id.absolute_row - 1, .column = begin.line_id.absolute_column};
    file->lsp_datas.computed->hover.range.pos2 =
      (Position){.row = end.file_id.absolute_row - 1, .column = end.line_id.absolute_column};
    file->lsp_datas.computed->hover.is_range = true;
  }

  // We assert that every data going outfrom there has a range.
  assert(file->lsp_datas.computed->hover.is_range);
  gui_resumeHoverInformation(cursor, view_port, &file->lsp_datas.computed->hover);

  free(payload);
}


void responseDispatcher(cJSON* packet, LSP_Server* lsp, DispatcherPayload* data) {
  LSP_PacketID id = LSP_getPacketID(packet);

  LSP_ResponseContext context;
  bool pop_result = LSP_popResponseContext(lsp, id, &context);
  assert(pop_result);

  int index = getIndexFileContainerForName(data, context.file_name);
  if (index == -1) {
    fprintf(stderr, "ERROR : Couldn't find the file for the current response.\n");
    exit(-1); // TODO remove
    return;
  }

  if (cJSON_GetObjectItem(packet, "error")) {
    if (strcmp(context.method, "textDocument/completion") == 0) {
      if (data->view_port.gui->edw_context.pow_owner == COMPLETION) {
        gui_closePopup(data->view_port.gui);
      }
    }
    fprintf(stderr, "LSP : ERROR RECEIVED from %s !\n    => Method issue : %s.\n    => Error message : %s\n",
            lsp->language, context.method,
            cJSON_GetStringValue(cJSON_GetObjectItem(cJSON_GetObjectItem(packet, "error"), "message")));
    return;
  }

  if (strcmp(context.method, "textDocument/completion") == 0) {
    fprintf(stderr, "RECEIVE completion !\n");
    receiveCompletionData(packet, data->files + index, &data->view_port, data->cursor);
  }
  else if (strcmp(context.method, "textDocument/hover") == 0) {
    fprintf(stderr, "RECEIVE hover\n");
    receiveHoverData(packet, data->files + index, &data->view_port, data->cursor, context.payload);
  }
  else {
    fprintf(stderr, "Response method NOT SUPPORTED !\n      => %s\n", context.method);
    exit(-1); // TODO remove
  }
}
