#ifndef WISHWIM_LSP_HOVER_H
#define WISHWIM_LSP_HOVER_H

#include "../../../terminal/term_handler.h"
#include "../lsp-features/lsp_completion.h"


void receiveHoverData(cJSON* packet, FileContainer* file, ViewPort* view_port, Cursor* cursor, void* payload);


#endif // WISHWIM_LSP_HOVER_H
