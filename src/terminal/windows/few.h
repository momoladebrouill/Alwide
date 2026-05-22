#ifndef WISHWIM_FEW_H
#define WISHWIM_FEW_H
#include "../../io-management/io_explorer.h"
#include "gui_entities.h"

void gui_initFEWContext(gui_FEW* context);

void gui_resizeFEW(gui_Context* gui_context, int few_new_width);

void gui_repaintFEW(gui_FEW* context, ExplorerFolder* pwd);

void gui_switchFEW(gui_Context* gui_context);

#endif // WISHWIM_FEW_H
