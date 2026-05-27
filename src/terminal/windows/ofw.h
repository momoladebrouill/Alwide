#ifndef WISHWIM_OFW_H
#define WISHWIM_OFW_H
#include "../../data-management/file_management.h"
#include "gui_entities.h"

void gui_initOFWContext(gui_OFW* context);

void gui_resizeOFW(gui_Context* gui_context);

void gui_repaintOFW(gui_OFW* context, FileContainer* files, int file_count, int current_file);

void gui_switchOFW(gui_Context* gui_context);

#endif
