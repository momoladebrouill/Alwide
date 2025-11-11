#ifndef WISHWIM_OFW_H
#define WISHWIM_OFW_H
#include "../../data-management/file_management.h"
#include "gui_entities.h"

void gui_initOFWContext(OFW_GUIContext* context);

void gui_resizeOFW(GUIContext* gui_context);

void gui_repaintOFW(OFW_GUIContext* context, FileContainer* files, int file_count, int current_file);

void gui_switchOFW(GUIContext* gui_context);

#endif // WISHWIM_OFW_H
