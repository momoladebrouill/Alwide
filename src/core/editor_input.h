#ifndef WISHWIM_EDITOR_INPUT_H
#define WISHWIM_EDITOR_INPUT_H
#include "../advanced/lsp/lsp_dispatcher.h"
#include "editor_context.h"

void handleCharBufferInsertion(EditorContext* ctx, int key);
bool handlePopupInput(EditorContext* ctx, int key);
int  readNextInput(EditorContext* ctx);
void logInput(int key);
EventLoopAction runSpecialKeyHandler(EditorContext* ctx, int key);
bool runInternalLogic(EditorContext* ctx, int key, EventLoopAction* out_action);
EventLoopAction dispatchInput(EditorContext* ctx, int key);


#endif
