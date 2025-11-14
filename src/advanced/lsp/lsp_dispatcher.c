
#include "lsp_dispatcher.h"

#include <assert.h>
#include <string.h>


void printPacket(cJSON* packet, cJSON* params) {
  if (LSP_getPacketType(packet) == NOTIFICATION) {
    fprintf(stderr, "\n\n <<< ================ %s ================\n",
            cJSON_GetStringValue(cJSON_GetObjectItem(packet, "method")));
  }
  else {
    fprintf(stderr, "\n\n <<< ================ %s ================\n",
            cJSON_GetStringValue(cJSON_GetObjectItem(packet, "id")));
  }
  char* text = cJSON_Print(params);
  fprintf(stderr, "%s\n", text);
  free(text);
}

int getIndexFileContainerForName(DispatcherPayload* payload, char* file_name) {
  for (int i = 0; i < payload->size; i++) {
    // fprintf(stderr, "Comparing :\n%s\n%s\n", payload->files[i].io_file.path_abs, uri + 7);
    if (strcmp(payload->files[i].io_file.path_abs, file_name) == 0) {
      // fprintf(stderr, "=> equals\n");
      return i;
    }
    else {
      // fprintf(stderr, "=> not equals\n");
    }
  }
  return -1;
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

  return getIndexFileContainerForName(payload, uri + 7);
}

void readPublishDiagnostic(cJSON* params, LSP_ComputedData* computed_data) {
  cJSON* diagnostics_item = cJSON_GetObjectItem(params, "diagnostics");
  int count = (computed_data->diagnostics_size = cJSON_GetArraySize(diagnostics_item));

  computed_data->diagnostics = realloc(computed_data->diagnostics, sizeof(Diagnostic) * count);

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
  LSP_ComputedData* computed_data = data->files[index].lsp_datas.computed;


  if (strcmp(method, "textDocument/publishDiagnostics") == 0) {
    readPublishDiagnostic(params, computed_data);
  }
  else {
    fprintf(stderr, "Method NOT SUPPORTED !\n      => %s\n", method);
    exit(-1); // TODO remove
  }
}


void responseDispatcher(cJSON* packet, LSP_Server* lsp, DispatcherPayload* data) {
  LSP_PacketID id = LSP_getPacketID(packet);

  LSP_ResponseContext context;
  LSP_popResponseContext(lsp, id, &context);

  int index = getIndexFileContainerForName(data, context.file_name);
  if (index == -1) {
    fprintf(stderr, "ERROR : Couldn't find the file for the current response.\n");
    exit(-1); // TODO remove
    return;
  }

  if (strcmp(context.method, "textDocument/completion") == 0) {
    // TODO implement the handle of the completion receive !
    fprintf(stderr, "RECEIVE completion !\n");
  }
  else {
    fprintf(stderr, "Reponse method NOT SUPPORTED !\n      => %s\n", context.method);
    exit(-1); // TODO remove
  }
}


void dispatcher(cJSON* packet, LSP_Server* lsp, void* payload) {
  DispatcherPayload* data = payload;
  printPacket(packet, packet);

  // Dispatcher
  switch (LSP_getPacketType(packet)) {
    case REQUEST:
      assert(false); // should be a request there !
      break;
    case NOTIFICATION:
      notificationDispatcher(packet, data);
      break;
    case RESPONSE:
      responseDispatcher(packet, lsp, data);
      break;
  }
}
