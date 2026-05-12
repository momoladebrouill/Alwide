#ifndef WISHWIM_EDITOR_INPUT_H
#define WISHWIM_EDITOR_INPUT_H
#include "editor_context.h"
bool handle_popup_input(EditorContext* ctx, int hash, int c, DispatcherPayload* payload);
void read_next_input(EditorContext* ctx, int* out_c, int* out_hash);
EventLoopAction process_key_event(EditorContext* ctx, int hash, int c);
#endif
