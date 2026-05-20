#ifndef WISHWIM_EDITOR_LSP_H
#define WISHWIM_EDITOR_LSP_H

#include "../advanced/lsp/lsp_dispatcher.h"
#include "editor_context.h"

ModuleContext buildModuleContext(EditorContext* ctx);
void handleLspServers(EditorContext* ctx, int* c, int* hash);
void waitForLspResponse(EditorContext* ctx, int timeout_ms);
void askOnCharTypeLspInfos(EditorContext* ctx, int c, FileContainer* fc, Cursor* cursor);

#endif
