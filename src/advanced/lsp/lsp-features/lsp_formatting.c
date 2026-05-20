#include "lsp_formatting.h"
#include <stdlib.h>
#include <string.h>
#include "../../../environnement/constants.h"
#include "../../../environnement/global_variables.h"
#include "../../../terminal/windows/edw.h"
#include "../lsp_handler.h"
#include "lsp_completion.h"
#include "lsp_tools.h"

void receiveFormattingData(cJSON* packet, FileContainer* file, ModuleContext* data) {
  cJSON* result = cJSON_GetObjectItem(packet, "result");
  if (result == NULL || cJSON_IsNull(result)) {
    return;
  }

  int edits_size = cJSON_GetArraySize(result);
  if (edits_size == 0) {
    return;
  }

  LSP_TextEdit* edits = malloc(edits_size * sizeof(LSP_TextEdit));
  for (int i = 0; i < edits_size; i++) {
    edits[i] = LSP_getTextEditFromJSON(cJSON_GetArrayItem(result, i));
  }

  applyTextEditsArray(data->cursor, edits, edits_size, &file->history_frame, data->payload_state_change,
                      ft_tab(file->feature));
  for (int i = 0; i < edits_size; i++) {
    LSP_destroyTextEdit(edits[i]);
  }
  free(edits);

  gui_updateEDW(data->view_port.gui);
}

void askFormatting(FileContainer* file) {
  if (!file->lsp_datas.is_enable) {
    return;
  }

  LSP_FormattingOptions options = {
    .tabSize = ft_tab_size(file->feature),
    .insertSpaces = ft_tab_use_space(file->feature),
    .trimTrailingWhitespace = true,
    .insertFinalNewline = true,
    .trimFinalNewlines = true,
  };

  LSP_Server* lsp = getLSPServerForLanguage(&lsp_servers, file->lsp_datas.lang_id);
  if (lsp == NULL) {
    return;
  }

  LSP_requestFormatting(lsp, file->io_file.path_abs, options);
}

void askOnTypeFormatting(FileContainer* file, char* ch, ModuleContext* data) {
  if (!file->lsp_datas.is_enable) {
    return;
  }
  LSP_Server* lsp = getLSPServerForLanguage(&lsp_servers, file->lsp_datas.lang_id);
  if (lsp == NULL) {
    return;
  }

  bool is_trigger = false;
  for (int i = 0; i < lsp->on_type_trigger_chars_count; i++) {
    if (strcmp(lsp->on_type_trigger_chars[i], ch) == 0) {
      is_trigger = true;
      break;
    }
  }

  if (is_trigger) {
    LSP_FormattingOptions options = {
      .tabSize = ft_tab_size(file->feature),
      .insertSpaces = ft_tab_use_space(file->feature),
      .trimTrailingWhitespace = true,
      .insertFinalNewline = true,
      .trimFinalNewlines = true,
    };

    LSP_requestOnTypeFormatting(lsp, file->io_file.path_abs,
                                LSP_pos(data->cursor->file_id.absolute_row - 1, data->cursor->line_id.absolute_column),
                                ch, options);
  }
}
