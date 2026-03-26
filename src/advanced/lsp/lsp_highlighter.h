#ifndef WISHWIM_LSP_HIGHLIGHTER_H
#define WISHWIM_LSP_HIGHLIGHTER_H

#include "../../terminal/highlight.h"
#include "../../terminal/windows/gui_entities.h"
#include "lsp_handler.h"


void LSP_highlightCurrentFile(LSP_Data* lsp_datas, Cursor cursor, WindowHighlightDescriptor* highlight_descriptor,
                              GUIContext* context);
#endif // WISHWIM_LSP_HIGHLIGHTER_H
