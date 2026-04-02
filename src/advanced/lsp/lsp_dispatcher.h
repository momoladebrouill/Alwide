#ifndef WISHWIM_LSP_DISPATCHER_H
#define WISHWIM_LSP_DISPATCHER_H

#include "../../../lib/cJSON/cJSON.h"

#include "../../data-management/file_management.h"
#include "../../io_management/viewport_history.h"

typedef struct DispatcherPayload {
  FilesState files_state;
  ViewPort view_port;
  Cursor *cursor;
} DispatcherPayload;


void dispatcher(cJSON* packet, LSP_Server* lsp, void* payload);

int getIndexFileContainerForUri(DispatcherPayload* payload, cJSON* params);

int getIndexFileContainerForName(DispatcherPayload* payload, char* file_name);

void printPacket(cJSON* packet, cJSON* params);

#endif // WISHWIM_LSP_DISPATCHER_H
