#ifndef WISHWIM_FEW_H
#define WISHWIM_FEW_H
#include "../../io-management/io_explorer.h"
#include "gui_entities.h"

void gui_initFEWContext(FEW_GUIContext* context);

void gui_resizeFEW(GUIContext* gui_context, int few_new_width);

void gui_repaintFEW(FEW_GUIContext* context, ExplorerFolder* pwd);

void switchFEW(GUIContext* gui_context);

#endif // WISHWIM_FEW_H
