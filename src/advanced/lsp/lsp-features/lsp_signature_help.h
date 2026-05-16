#ifndef LSP_SIGNATURE_HELP_H
#define LSP_SIGNATURE_HELP_H

#include "../../../core/editor_context.h"
#include "../lsp_client.h"

void receiveSignatureHelpData(cJSON* packet, FileContainer* file, GUIContext* gui, Cursor* cursor);

void askSignatureHelp(FileContainer* file, Cursor* cursor);

void adaptSignatureHelp(Cursor* cursor, LSP_Data* lsp_data);


#endif // LSP_SIGNATURE_HELP_H
