#include "lsp_code_action.h"
#include <stdlib.h>
#include <string.h>
#include "../../../environnement/constants.h"
#include "../../../environnement/global_variables.h"
#include "../../../terminal/windows/pow.h"
#include "lsp_tools.h"

/**
 * Processes the response from 'textDocument/codeAction'.
 * Parses the available actions and opens the TUI selection popup.
 */
void receiveCodeActionData(cJSON* packet, FileContainer* file, ModuleContext* data) {
  cJSON* result = cJSON_GetObjectItem(packet, "result");
  if (result == NULL || cJSON_IsNull(result)) {
    return;
  }

  LSP_ComputedData* computed = file->lsp_datas.computed;
  
  // 1. Clean up old code actions before storing new ones
  for (int i = 0; i < computed->code_actions_size; i++) {
    LSP_destroyCodeAction(computed->code_actions + i);
  }
  free(computed->code_actions);

  // 2. Parse new actions from JSON
  computed->code_actions_size = cJSON_GetArraySize(result);
  if (computed->code_actions_size == 0) {
    computed->code_actions = NULL;
    return;
  }

  computed->code_actions = malloc(sizeof(LSP_CodeAction) * computed->code_actions_size);
  for (int i = 0; i < computed->code_actions_size; i++) {
    computed->code_actions[i] = LSP_getCodeActionFromJSON(cJSON_GetArrayItem(result, i));
  }

  // 3. Open the selection popup near the cursor
  ViewPort view_port = viewPortOf(data->view_port.gui, &file->screen_x, &file->screen_y);
  gui_showGenericPopupWithTextAnchor(&view_port, data->cursor, computed->code_actions_size + 2, 45, CODE_ACTION);
  gui_updateEDW(data->view_port.gui);
}

/**
 * Initiates a code action request.
 * Gathers diagnostics on the current line to provide context to the LSP server.
 */
void askCodeAction(FileContainer* file, Cursor* cursor) {
  if (!file->lsp_datas.is_enable) return;

  LSP_Server* lsp = getLSPServerForLanguage(&lsp_servers, file->lsp_datas.lang_id);
  if (lsp == NULL) return;

  LSP_ComputedData* computed = file->lsp_datas.computed;
  int lsp_row = cursor_row(*cursor) - 1;
  
  // Gathers up to 10 diagnostics overlapping the current line to help the server
  // find relevant quick-fixes.
  LSP_Diagnostic context_diags[10];
  int diags_count = 0;

  for (int i = 0; i < computed->diagnostics_size && diags_count < 10; i++) {
    LSP_Diagnostic* d = &computed->diagnostics[i];
    if (d->range.pos1.row <= lsp_row && d->range.pos2.row >= lsp_row) {
      context_diags[diags_count++] = *d;
    }
  }

  // Request code actions for the whole current line
  LSP_Range range = LSP_range(LSP_pos(lsp_row, 0), LSP_pos(lsp_row, 1000));

  LSP_requestCodeAction(lsp, file->io_file.path_abs, range, context_diags, diags_count);
}

/**
 * Finalizes the code action by applying its WorkspaceEdit.
 * delegates actual text manipulation to lsp_tools.c.
 */
void executeCodeAction(FileContainer* fc, Cursor* cursor, LSP_CodeAction* action, ModuleContext* data) {
  if (!action) return;

  if (action->has_edit) {
      // Apply the WorkspaceEdit (handled in lsp_tools.c to support multi-edit/multi-file shifts)
      applyWorkspaceEdit(fc, cursor, &action->edit, data->payload_state_change);
  }
  
  if (action->has_command) {
      LSP_Server* lsp = getLSPServerForLanguage(&lsp_servers, fc->lsp_datas.lang_id);
      if (lsp) {
          LSP_requestExecuteCommand(lsp, fc->io_file.path_abs, &action->command);
      }
  }

  gui_updateEDW(data->view_port.gui);
}
