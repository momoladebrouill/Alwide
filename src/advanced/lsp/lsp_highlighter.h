#ifndef WISHWIM_LSP_HIGHLIGHTER_H
#define WISHWIM_LSP_HIGHLIGHTER_H

#include "../../terminal/highlight.h"
#include "lsp_handler.h"

void LSP_highlightCurrentFile(LSP_Data* lsp_datas, Cursor cursor, WindowHighlightDescriptor* highlight_descriptor);

#endif // WISHWIM_LSP_HIGHLIGHTER_H
