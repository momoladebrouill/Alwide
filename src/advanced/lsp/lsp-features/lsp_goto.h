#ifndef WISHWIM_LSP_GOTO_H
#define WISHWIM_LSP_GOTO_H
#include "../lsp_dispatcher.h"

void receiveGotoData(cJSON* packet, LSP_Server* lsp, FileContainer* file, ModuleContext* data,
                     LSP_LocationArray* location_array, char* method, void* payload);

void jumpToLocation(ModuleContext* data, LSP_Location location);

#endif //WISHWIM_LSP_GOTO_H
