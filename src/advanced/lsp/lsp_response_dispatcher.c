#include "lsp_response_dispatcher.h"

#include <assert.h>
#include <string.h>

#include "../../terminal/term_handler.h"
#include "../../terminal/windows/edw.h"
#include "../../terminal/windows/pow.h"
#include "lsp_features/lsp_completion.h"
#include "lsp_features/lsp_goto.h"
#include "lsp_features/lsp_hover.h"


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

  FileContainer* file = (*data->files_state.files) + index;

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
    receiveCompletionData(packet, file, &data->view_port, data->cursor);
  }
  else if (strcmp(context.method, "textDocument/hover") == 0) {
    receiveHoverData(packet, file, &data->view_port, data->cursor, context.payload);
  }
  else if (strcmp(context.method, "textDocument/definition") == 0 ||
           strcmp(context.method, "textDocument/declaration") == 0 ||
           strcmp(context.method, "textDocument/typeDefinition") == 0 ||
           strcmp(context.method, "textDocument/implementation") == 0 ||
           strcmp(context.method, "textDocument/references") == 0) {
    receiveGotoData(packet, lsp, file, data, &file->lsp_datas.computed->gotos, context.method, context.payload);
  }
  else {
    fprintf(stderr, "Response method NOT SUPPORTED !\n      => %s\n", context.method);
    exit(-1); // TODO remove
  }
}
