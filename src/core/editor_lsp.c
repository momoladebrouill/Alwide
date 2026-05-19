#include "editor_lsp.h"

#include <ncurses.h>
#include <unistd.h>

#include "../advanced/lsp/lsp-features/lsp_completion.h"
#include "../advanced/lsp/lsp-features/lsp_signature_help.h"
#include "../advanced/lsp/lsp_client.h"
#include "../environnement/global_variables.h"
#include "../utils/key_management.h"
#include "../utils/tools.h"

ModuleContext buildModuleContext(EditorContext* ctx) {
  FileContainer* fc = &ctx->files[ctx->current_file_index];
  ModuleContext payload;
  payload.files_state = filesStateOf(&ctx->files, &ctx->file_count, &ctx->current_file_index, &ctx->refresh_local_vars);
  payload.view_port = viewPortOf(&ctx->gui_context, &fc->screen_x, &fc->screen_y);
  payload.cursor = &fc->cursor;
  payload.payload_state_change = ctx->payload_state_change;
  return payload;
}

void handleLspServers(EditorContext* ctx, int* c, int* hash) {
  LSPServerLinkedList_Cell* cell = lsp_servers.head;
  while (cell != NULL) {
    ModuleContext payload = buildModuleContext(ctx);
    while (LSP_dispatchOnReceive(&cell->lsp_server, dispatcher, &payload)) {
      if (*c == ERR) {
        *c = ONLY_REPAINT_INPUT;
        *hash = ONLY_REPAINT_INPUT;
      }
      // Refresh payload in case of realloc during dispatch
      payload = buildModuleContext(ctx);
    }
    cell = cell->next;
  }
}

void waitForLspResponse(EditorContext* ctx, int timeout_ms) {
  time_val start = timeInMilliseconds();
  int dummy_c = ERR, dummy_hash = ERR;
  while (diff2Time(timeInMilliseconds(), start) < timeout_ms) {
    handleLspServers(ctx, &dummy_c, &dummy_hash);
    usleep(10000); // 10ms
  }
}

void askOnCharTypeLspInfos(EditorContext* ctx, int c, FileContainer* fc, Cursor* cursor) {
  bool hasAsked = askSignatureHelpOnChar(ctx, c, fc, cursor); // priority 1
  if (hasAsked) {
    return;
  }
  askCompletion(&ctx->gui_context, fc, false, false); // priority 2
}
