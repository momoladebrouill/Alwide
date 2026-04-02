#ifndef WISHWIM_LSP_COMPLETION_H
#define WISHWIM_LSP_COMPLETION_H

#include "../../../data-management/file_management.h"
#include "../../../data-management/file_structure.h"
#include "../../../data-management/state_control.h"
#include "../../../environnement/global-variables.h"
#include "../../../io_management/viewport_history.h"
#include "../../shared.h"
#include "../lsp_client.h"

void LSP_executeCompletion(Cursor* cursor, LSP_CompletionItem* item, History** history_p,
                           PayloadStateChange payload_state_change);

void askCompletion(GUIContext* gui_context, Cursor* cursor, int* screen_x, int* screen_y, LSP_Data* lsp_data,
                   bool reset, bool force);
void receiveCompletionData(cJSON* packet, FileContainer* file, ViewPort* view_port, Cursor* cursor);

#endif // WISHWIM_LSP_COMPLETION_H
