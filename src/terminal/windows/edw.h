#ifndef WISHWIM_FTW_H
#define WISHWIM_FTW_H

#include "../../advanced/lsp/lsp_handler.h"
#include "../highlight.h"
#include "gui_entities.h"

void gui_initEDWContext(EDW_GUIContext* context);

void gui_resizeEDW(GUIContext* gui_context, int lnw_new_width);

void gui_repaintEDW(EDW_GUIContext* context, Cursor cursor, Cursor select_cursor, int screen_x, int screen_y,
                    WindowHighlightDescriptor* highlight_descriptor, LSP_ComputedData* lsp_data, int tab_size);

int getEDW_LengthLineNumber(GUIContext* gui_context);

bool gui_showPopup(GUIContext* gui_context, int y, int x, int height, int width, PopupOwner owner);

void gui_closePopup(GUIContext* gui_context);

bool gui_adaptPopup(GUIContext* gui_context, int slice_x, int slice_y);

#endif // WISHWIM_FTW_H
