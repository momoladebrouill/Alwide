#include "few.h"

#include <libgen.h>
#include <ncurses.h>

#include "../../environnement/constants.h"
#include "../../utils/tools.h"
#include "edw.h"
#include "ofw.h"


void gui_initFEWContext(gui_FEW* context) {
  context->few = NULL;         // File Explorer Window
  context->refresh_few = true; // Need to reprint file explorer window

  context->few_width = 0; // File explorer width
  context->saved_few_width = FILE_EXPLORER_WIDTH;
  context->few_x_offset = 0; /* TODO unused */
  context->few_y_offset = 0; // Y Scroll state of File Explorer Window
  context->few_selected_line = -1;
}


void gui_resizeFEW(gui_Context* gui_context, int few_new_width) {
  if (few_new_width == -1) {
    few_new_width = gui_context->few_context.few_width;
  }
  few_new_width = max(0, few_new_width);
  gui_context->few_context.few_width = few_new_width;
  // Resize File Explorer Window
  delwin(gui_context->few_context.few);
  // We don't allocate a window if the width is NULL.
  if (gui_context->few_context.few_width == 0) {
    gui_context->few_context.few = NULL;
  }
  else {
    gui_context->few_context.few = newwin(0, gui_context->few_context.few_width, 0, 0);
    wbkgd(gui_context->few_context.few, COLOR_PAIR(DEFAULT_COLOR_PAIR));
  }

  // Resize Opened File Window
  gui_resizeOFW(gui_context);
  // Resize Editor Window
  gui_resizeEDW(gui_context, -1);
  gui_context->few_context.refresh_few = true;
}

void gui_switchFEW(gui_Context* gui_context) {
  if (gui_context->few_context.few == NULL) {
    // Open File Explorer Window
    gui_context->few_context.few_width = gui_context->few_context.saved_few_width;
    gui_context->few_context.few = newwin(0, gui_context->few_context.few_width, 0, 0);
    gui_context->few_context.refresh_few = true;
    wbkgd(gui_context->few_context.few, COLOR_PAIR(DEFAULT_COLOR_PAIR));
  }
  else {
    // Close File Explorer Window
    gui_context->few_context.saved_few_width = getmaxx(gui_context->few_context.few);
    delwin(gui_context->few_context.few);
    gui_context->few_context.few = NULL;
    gui_context->few_context.few_width = 0;
  }
  // Resize Opened File Window
  gui_resizeOFW(gui_context);
  // Resize Editor Window
  gui_resizeEDW(gui_context, -1);
}


void internalPrintExplorerRec(ExplorerFolder* folder, WINDOW* few, int* few_x_offset, int* few_y_offset,
                              int tree_offset_rec, int* selected_line) {
  // Don't print if not in window.
  if (getcury(few) + 1 >= getmaxy(few)) {
    return;
  }

  if (folder->open && folder->discovered == false) {
    discoverFolder(folder);
  }

  (*selected_line)--;
  if (*few_y_offset == 0) {
    // Print current folder name

    if (*selected_line == 0) {
      wattron(few, A_STANDOUT);
    }

    for (int i = 0; i < tree_offset_rec && i < getmaxx(few); i++) {
      printToNcursesNCharFromString(few, " ", getmaxx(few) - (getcurx(few) + 1));
    }

    // Print decoration of folder. The decoration describe if the folder is open or not.
    if (folder->open) {
      printToNcursesNCharFromString(few, "⌄", getmaxx(few) - (getcurx(few) + 1));
    }
    else {
      printToNcursesNCharFromString(few, "›", getmaxx(few) - (getcurx(few) + 1));
    }

    printToNcursesNCharFromString(few, "📁", getmaxx(few) - (getcurx(few) + 1));
    printToNcursesNCharFromString(few, basename(folder->path), getmaxx(few) - (getcurx(few) + 1));
    if (*selected_line == 0) {
      for (int j = getcurx(few) + 1; j < getmaxx(few); j++) {
        printToNcursesNCharFromString(few, " ", getmaxx(few) - (getcurx(few) + 1));
      }
      wattroff(few, A_STANDOUT);
    }
    wprintw(few, "\n");
  }
  else {
    (*few_y_offset)--;
  }

  if (folder->open == false) {
    return;
  }

  // Print sub folders
  for (int i = 0; i < folder->folder_count; i++) {
    internalPrintExplorerRec(folder->folders + i, few, few_x_offset, few_y_offset,
                             tree_offset_rec + FILE_EXPLORER_TREE_OFFSET, selected_line);
  }
  // Print sub files
  for (int i = 0; i < folder->file_count; i++) {
    // Don't print if not in window.
    if (getcury(few) + 1 >= getmaxy(few)) {
      return;
    }

    (*selected_line)--;
    if (*few_y_offset == 0) {
      // Print file name
      if (*selected_line == 0) {
        wattron(few, A_STANDOUT);
      }
      for (int j = 0;
           j < tree_offset_rec + FILE_EXPLORER_TREE_OFFSET + 1 /*Add one to balance with the folder decoration*/; j++) {
        printToNcursesNCharFromString(few, " ", getmaxx(few) - (getcurx(few) + 1));
      }
      printToNcursesNCharFromString(few, "📄", getmaxx(few) - (getcurx(few) + 1));
      printToNcursesNCharFromString(few, basename(folder->files[i].path), getmaxx(few) - (getcurx(few) + 1));
      if (*selected_line == 0) {
        for (int j = getcurx(few) + 1; j < getmaxx(few); j++) {
          printToNcursesNCharFromString(few, " ", getmaxx(few) - (getcurx(few) + 1));
        }
        wattroff(few, A_STANDOUT);
      }
      wprintw(few, "\n");
    }
    else {
      (*few_y_offset)--;
    }
  }
}

void gui_repaintFEW(gui_FEW* context, ExplorerFolder* pwd) {
  if (!(context->refresh_few == true && context->few_width != 0 && context->few != NULL)) {
    return;
  }
  werase(context->few);
  wmove(context->few, 0, 0);

  // the internal fct need edit this var to lake them.
  // We need to make a copy of them to keep the value right in the gui_context.
  int tmp_few_x_offset = context->few_x_offset;
  int tmp_few_y_offset = context->few_y_offset;
  int tmp_few_selected_line = context->few_selected_line;
  internalPrintExplorerRec(pwd, context->few, &tmp_few_x_offset, &tmp_few_y_offset, 0, &tmp_few_selected_line);
  // Clear end of window

  for (int i = getbegy(context->few); i < getmaxy(context->few); i++) {
    mvwprintw(context->few, i, getmaxx(context->few) - 1, "│");
  }

  wnoutrefresh(context->few);
  context->refresh_few = false;
}
