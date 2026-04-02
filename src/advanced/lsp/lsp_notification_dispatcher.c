#include "lsp_notification_dispatcher.h"

#include <string.h>


void readPublishDiagnostic(cJSON* params, LSP_ComputedData* computed_data) {
  cJSON* diagnostics_item = cJSON_GetObjectItem(params, "diagnostics");
  int count = (computed_data->diagnostics_size = cJSON_GetArraySize(diagnostics_item));

  computed_data->diagnostics = realloc(computed_data->diagnostics, sizeof(LSP_Diagnostic) * count);

  for (int i = 0; i < count; i++) {
    computed_data->diagnostics[i] = LSP_getDiagnosticFromJSON(cJSON_GetArrayItem(diagnostics_item, i));
  }
}


void notificationDispatcher(cJSON* packet, DispatcherPayload* data) {
  char* method = LSP_getPacketMethod(packet);
  cJSON* params = cJSON_GetObjectItem(packet, "params");

  // fetch computed data
  int index = getIndexFileContainerForUri(data, params);
  if (index == -1) {
    fprintf(stderr, "ERROR : Couldn't find the file for the current packet.\n");
    exit(-1); // TODO remove
    return;
  }
  LSP_ComputedData* computed_data = (*data->files_state.files)[index].lsp_datas.computed;

  if (strcmp(method, "textDocument/publishDiagnostics") == 0) {
    readPublishDiagnostic(params, computed_data);
  }
  else {
    fprintf(stderr, "Method NOT SUPPORTED !\n      => %s\n", method);
    exit(-1); // TODO remove
  }
}