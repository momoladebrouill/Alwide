#ifndef LSP_FORMATTING_H
#define LSP_FORMATTING_H

#include "../lsp_dispatcher.h"
#include "../lsp_client.h"

void receiveFormattingData(cJSON* packet, FileContainer* file, ModuleContext* data);

void askFormatting(FileContainer* file);

void askOnTypeFormatting(FileContainer* file, char* ch, ModuleContext* data);

#endif // LSP_FORMATTING_H
