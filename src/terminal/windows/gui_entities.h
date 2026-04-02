#ifndef WISHWIM_GUI_ENTITIES_H
#define WISHWIM_GUI_ENTITIES_H
#include <ncurses.h>

#include "../../data-management/file_structure.h"

typedef struct {
  // NCurses items
  WINDOW* ofw; // Opened Files Window

  // Local data
  bool refresh_ofw; // Need to reprint opened file window

  int current_file_offset;
  int ofw_height; // Height of Opened Files Window. 0 => Disabled on start.   OPENED_FILE_WINDOW_HEIGHT => Enabled on
  // start.
} OFW_GUIContext;

typedef struct {
  // NCurses items
  WINDOW* few; // File Explorer Window

  // Local data
  bool refresh_few; // Need to reprint file explorer window

  int few_width; // File explorer width
  int saved_few_width;
  int few_x_offset; /* TODO unused */
  int few_y_offset; // Y Scroll state of File Explorer Window
  int few_selected_line;
} FEW_GUIContext;


typedef enum { DIAGNOSTICS, COMPLETION, HOVER_DIAGNOSTICS, GOTO_CHOICE, NO_OWNER } PopupOwner;

typedef struct {
  // NCurses items
  WINDOW* ftw; // File Text Window
  WINDOW* lnw; // Line Number Window
  WINDOW* pow; // Popup Window

  // Local data
  bool refresh_edw; // Need to reprint editor window

  // lnw vars
  int length_line_number;

  // Popup vars
  bool show_pow;
  PopupOwner pow_owner;
  int item_selected;
  int item_select_offset_y;
  CursorDescriptor lastTextAnchor;
  CursorDescriptor lastMousePosition;
} EDW_GUIContext;


typedef struct {
  // Init GUI vars
  EDW_GUIContext edw_context; // Editor Window Context
  OFW_GUIContext ofw_context; // Opened File Context
  FEW_GUIContext few_context; // File Explorer Context

  // Used to set the window where start mouse drag
  WINDOW* focus_w;
} GUIContext;


// // TODO implement a toplevel popup
// typedef struct {
//   WINDOW* pow;      // Popup Window
//   bool refresh_pow; // Need to repaint the current popup
//
//
// } GUIPopup;


#endif // WISHWIM_GUI_ENTITIES_H
