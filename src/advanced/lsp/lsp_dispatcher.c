
#include "lsp_dispatcher.h"

#include <assert.h>
#include <string.h>

#include "../../utils/tools.h"
#include "lsp_notification_dispatcher.h"
#include "lsp_response_dispatcher.h"

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

int getIndexFileContainerForName(DispatcherPayload* payload, char* file_name) {
  for (int i = 0; i < *payload->files_state.size; i++) {
    if (strcmp((*payload->files_state.files)[i].io_file.path_abs, file_name) == 0) {
      return i;
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

  char decoded_path[PATH_MAX];
  decodeURI(uri, decoded_path, PATH_MAX);

  return getIndexFileContainerForName(payload, decoded_path);
}


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
