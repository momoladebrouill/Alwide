#ifndef ALWIDE_TOPLEVEL_POPUP_H
#define ALWIDE_TOPLEVEL_POPUP_H

#include "gui_entities.h"

gui_TPW* gui_createToplevelPopup(gui_Context* gui_context, int y, int x, int height, int width,
                                 gui_TPW_paintCallback paint_cb, gui_TPW_inputCallback input_cb,
                                 gui_TPW_destroyCallback destroy_cb, void* payload);

void gui_repaintTPW(gui_Context *gui_context);

void gui_updateTPW(gui_Context *gui_context);

void gui_destroyToplevelPopup(gui_Context* gui_context, gui_TPW* popup);
void gui_destroyAllToplevelPopups(gui_Context* gui_context);
void gui_setToplevelPopupFocus(gui_Context* gui_context, gui_TPW* popup);

#endif // ALWIDE_TOPLEVEL_POPUP_H
