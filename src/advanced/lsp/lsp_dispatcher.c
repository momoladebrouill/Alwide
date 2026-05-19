
#include "lsp_dispatcher.h"

#include <assert.h>
#include <string.h>
#include <libgen.h>

#include "../../utils/tools.h"
#include "../../terminal/term_handler.h"
#include "lsp_notification_dispatcher.h"
#include "lsp_response_dispatcher.h"

#include "lsp-features/lsp_tools.h"

void handleServerRequest(cJSON* packet, LSP_Server* lsp, ModuleContext* data) {
    char* method = LSP_getPacketMethod(packet);
    LSP_PacketID id = LSP_getPacketID(packet);
    cJSON* params = cJSON_GetObjectItem(packet, "params");

    if (strcmp(method, "workspace/applyEdit") == 0) {
        cJSON* edit_json = cJSON_GetObjectItem(params, "edit");
        LSP_WorkspaceEdit edit = LSP_getWorkspaceEditFromJSON(edit_json);
        
        // Find the active file to apply the edit
        // (In a more complex setup, we'd find the FileContainer per URI)
        int current_idx = getIndexFileContainerForName(data, data->view_port.gui->ofw_context.ofw ? "" : ""); // dummy for now
        // Let's use getActiveFile equivalent logic
        FileContainer* active_fc = &(*data->files_state.files)[0]; // Fallback to first if index logic is complex
        // Actually, we can just try to apply it to all open files that match the URI in applyWorkspaceEdit
        for(int i=0; i<*data->files_state.size; i++) {
            applyWorkspaceEdit(&(*data->files_state.files)[i], data->cursor, &edit, data->payload_state_change);
        }

        LSP_destroyWorkspaceEdit(&edit);

        // Respond to server: { applied: true }
        cJSON* result = cJSON_CreateObject();
        cJSON_AddBoolToObject(result, "applied", true);
        LSP_sendResponse(lsp, id, result);
        
        gui_updateGUI(data->view_port.gui);
    } else {
        fprintf(stderr, "Server request NOT SUPPORTED: %s\n", method);
        // Minimum response for unknown requests
        LSP_sendResponse(lsp, id, NULL);
    }
}

void dispatcher(cJSON* packet, LSP_Server* lsp, void* payload) {
  ModuleContext* data = payload;
  printPacket(packet, packet);

  // Dispatcher
  switch (LSP_getPacketType(packet)) {
    case LSP_REQUEST:
      handleServerRequest(packet, lsp, data);
      break;
    case LSP_NOTIFICATION:
      notificationDispatcher(packet, data);
      break;
    case LSP_RESPONSE:
      responseDispatcher(packet, lsp, data);
      break;
  }
}

int getIndexFileContainerForName(ModuleContext* payload, char* file_name) {
  for (int i = 0; i < *payload->files_state.size; i++) {
    if (strcmp((*payload->files_state.files)[i].io_file.path_abs, file_name) == 0) {
      return i;
    }
  }
  return -1;
}


int getIndexFileContainerForUri(ModuleContext* payload, cJSON* params) {
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

  char canonical_path[PATH_MAX];
  if (realpath(decoded_path, canonical_path) != NULL) {
    return getIndexFileContainerForName(payload, canonical_path);
  }

  // If realpath fails (e.g. file doesn't exist), try to canonicalize the directory
  char path_copy[PATH_MAX];
  strncpy(path_copy, decoded_path, PATH_MAX);
  char* dname = dirname(path_copy);
  char dir_abs[PATH_MAX];
  if (realpath(dname, dir_abs) != NULL) {
    char bname_copy[PATH_MAX];
    strncpy(bname_copy, decoded_path, PATH_MAX);
    char* bname = basename(bname_copy);
    snprintf(canonical_path, PATH_MAX, "%s/%s", dir_abs, bname);
    return getIndexFileContainerForName(payload, canonical_path);
  }

  return getIndexFileContainerForName(payload, decoded_path);
}


void printPacket(cJSON* packet, cJSON* params) {
  if (LSP_getPacketType(packet) == LSP_NOTIFICATION) {
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
