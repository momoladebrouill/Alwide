
#include "lsp_dispatcher.h"

#include <string.h>


void printPacket(cJSON* packet, cJSON* params) {
  fprintf(stderr, "\n\n <<< ================ %s ================\n",
          cJSON_GetStringValue(cJSON_GetObjectItem(packet, "method")));
  char* text = cJSON_Print(params);
  fprintf(stderr, "%s\n", text);
  free(text);
}


int getIndexFileContainerForUri(DispatcherPayload* payload, cJSON* params) {
  cJSON* uri_item = cJSON_GetObjectItem(params, "uri");
  if (!uri_item) {
    return -1;
  }
  char* uri;
  if (!((uri = cJSON_GetStringValue(uri_item)))) {
    return -1;
  }

  for (int i = 0; i < payload->size; i++) {
    // fprintf(stderr, "Comparing :\n%s\n%s\n", payload->files[i].io_file.path_abs, uri + 7);
    if (strcmp(payload->files[i].io_file.path_abs, uri + 7) == 0) {
      // fprintf(stderr, "=> equals\n");
      return i;
    }
    else {
      // fprintf(stderr, "=> not equals\n");
    }
  }
  return -1;
}

void readPublishDiagnostic(cJSON* params, LSP_ComputedData* computed_data) {
  cJSON* diagnostics_item = cJSON_GetObjectItem(params, "diagnostics");
  int count = (computed_data->diagnostics_size = cJSON_GetArraySize(diagnostics_item));

  computed_data->diagnostics = realloc(computed_data->diagnostics, sizeof(Diagnostic) * count);

  for (int i = 0; i < count; i++) {
    computed_data->diagnostics[i] = LSP_getDiagnosticFromJSON(cJSON_GetArrayItem(diagnostics_item, i));
  }
}

void dispatcher(cJSON* packet, void* payload) {
  DispatcherPayload* data = payload;
  printPacket(packet, packet);

  // Dispatcher
  char* method = cJSON_GetStringValue(cJSON_GetObjectItem(packet, "method"));
  cJSON* params = cJSON_GetObjectItem(packet, "params");

  // fetch computed data
  int index = getIndexFileContainerForUri(payload, params);
  if (index == -1) {
    fprintf(stderr, "ERROR : Couldn't find the file for the current packet.\n");
    exit(-1); // TODO remove
    return;
  }
  LSP_ComputedData* computed_data = data->files[index].lsp_datas.computed;


  if (strcmp(method, "textDocument/publishDiagnostics") == 0) {
    readPublishDiagnostic(params, computed_data);
  }
  else {
    fprintf(stderr, "Method NOT SUPPORTED !\n      => %s\n", method);
    exit(-1); // TODO remove
  }
}
