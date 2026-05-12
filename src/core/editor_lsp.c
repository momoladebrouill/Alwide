#include "editor_lsp.h"
#include "../environnement/global-variables.h"
#include "../advanced/lsp/lsp_client.h"
#include "../utils/key_management.h"
#include <ncurses.h>
DispatcherPayload build_dispatcher_payload(EditorContext* ctx) {
  FileContainer* fc = &ctx->files[ctx->current_file_index];
  DispatcherPayload payload;
  payload.files_state = filesStateOf(&ctx->files, &ctx->file_count, &ctx->current_file_index, &ctx->refresh_local_vars);
  payload.view_port = viewPortOf(&ctx->gui_context, &fc->screen_x, &fc->screen_y);
  payload.cursor = &fc->cursor;
  return payload;
}

void dispatch_lsp(DispatcherPayload* payload, int* c, int* hash) {
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
