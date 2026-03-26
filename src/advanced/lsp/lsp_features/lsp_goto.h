#ifndef WISHWIM_LSP_GOTO_H
#define WISHWIM_LSP_GOTO_H
#include "../lsp_dispatcher.h"

void receiveGotoData(cJSON* packet, FileContainer* file, DispatcherPayload* data, LocationArray* location_array,
                         char* method) ;

#endif //WISHWIM_LSP_GOTO_H
