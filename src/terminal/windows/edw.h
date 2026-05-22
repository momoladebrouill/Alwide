#ifndef WISHWIM_FTW_H
#define WISHWIM_FTW_H

#include "../../advanced/lsp/lsp_handler.h"
#include "../highlight.h"
#include "gui_entities.h"

void gui_initEDWContext(gui_EDW* context);

void gui_resizeEDW(gui_Context* gui_context, int lnw_new_width);

void gui_repaintEDW(gui_EDW* context, Cursor cursor, Cursor select_cursor, int screen_x, int screen_y,
                    WindowHighlightDescriptor* highlight_descriptor, LSP_ComputedData* lsp_data, int tab_size);

int getEDW_LengthLineNumber(gui_Context* gui_context);

bool gui_showPopup(gui_Context* gui_context, int y, int x, int height, int width, PopupOwner owner);

void gui_closePopup(gui_Context* gui_context);

bool gui_adaptPopup(gui_Context* gui_context, int slice_x, int slice_y);

#endif // WISHWIM_FTW_H
