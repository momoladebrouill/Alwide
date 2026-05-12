#ifndef WISHWIM_EDITOR_LSP_H
#define WISHWIM_EDITOR_LSP_H
#include "editor_context.h"
DispatcherPayload build_dispatcher_payload(EditorContext* ctx);
void dispatch_lsp(DispatcherPayload* payload, int* c, int* hash);
#endif
