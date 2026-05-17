#ifndef LSP_SIGNATURE_HELP_H
#define LSP_SIGNATURE_HELP_H

#include "../../../core/editor_context.h"
#include "../lsp_client.h"

void receiveSignatureHelpData(cJSON* packet, FileContainer* file, GUIContext* gui, Cursor* cursor);

void askSignatureHelp(FileContainer* file, Cursor* cursor);

bool adaptSignatureHelpOnDelete(Cursor cursor, Cursor select_cursor, LSP_Data* lsp_data, EditorContext* ctx);

bool askSignatureHelpOnChar(EditorContext* ctx, int c, FileContainer* fc, Cursor* cursor);

#endif // LSP_SIGNATURE_HELP_H
