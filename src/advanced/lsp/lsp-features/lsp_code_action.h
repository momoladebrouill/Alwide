#ifndef LSP_CODE_ACTION_H
#define LSP_CODE_ACTION_H

#include "../lsp_client.h"
#include "../lsp_dispatcher.h"

/**
 * Handles the response from textDocument/codeAction.
 */
void receiveCodeActionData(cJSON* packet, FileContainer* file, ModuleContext* data);

/**
 * Requests code actions for the current cursor position.
 * It will automatically find diagnostics overlapping with the cursor line/position.
 */
void askCodeAction(FileContainer* file, Cursor* cursor);

/**
 * Executes a specific CodeAction by applying its WorkspaceEdit.
 */
void executeCodeAction(FileContainer* fc, Cursor* cursor, LSP_CodeAction* action, ModuleContext* data);

#endif // LSP_CODE_ACTION_H
