#ifndef WISHWIM_EDITOR_INPUT_H
#define WISHWIM_EDITOR_INPUT_H
#include "editor_context.h"
#include "../advanced/lsp/lsp_dispatcher.h"


bool handlePopupInput(EditorContext* ctx, int c, int hash);
void readNextInput(EditorContext* ctx, int* out_c, int* out_hash);
EventLoopAction runKeyHandler(EditorContext* ctx, int c, int hash);


#endif
