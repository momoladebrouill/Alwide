#include "tpw.h"
#include <stdlib.h>
#include "../../environnement/constants.h"
#include "../term_handler.h"

gui_TPW* gui_createToplevelPopup(gui_Context* gui_context, int y, int x, int height, int width,
                                 gui_TPW_paintCallback paint_cb, gui_TPW_inputCallback input_cb,
                                 gui_TPW_destroyCallback destroy_cb, void* payload) {

  gui_TPW* tpw_context = malloc(sizeof(gui_TPW));
  if (!tpw_context) {
    return NULL;
  }

  tpw_context->tpw = newwin(height, width, y, x);
  tpw_context->y = y;
  tpw_context->x = x;
  tpw_context->height = height;
  tpw_context->width = width;
  tpw_context->visible = true;
  tpw_context->has_focus = true; // Focus by default on creation
  tpw_context->on_paint = paint_cb;
  tpw_context->on_input = input_cb;
  tpw_context->on_destroy = destroy_cb;
  tpw_context->payload = payload;
  tpw_context->next = NULL;
  tpw_context->strong_focus = true;

  // TODO extract this to a method
  // Unfocus all other popups
  gui_TPW* curr = gui_context->toplevel_popups;
  while (curr != NULL) {
    curr->has_focus = false;
    curr = curr->next;
  }

  // TODO extract this to a method gui_addNewPopup()
  // Prepend to the linked list
  tpw_context->next = gui_context->toplevel_popups;
  gui_context->toplevel_popups = tpw_context;
  gui_updateTPW(gui_context);


  // Style the window
  if (tpw_context->tpw) {
    wbkgd(tpw_context->tpw, COLOR_PAIR(INFO_COLOR_PAIR));
    keypad(tpw_context->tpw, TRUE);
  }

  return tpw_context;
}

void gui_repaintTPW(gui_Context *gui_context) {
  // Paint each active toplevel popup
  gui_TPW* popup = gui_context->toplevel_popups;
  while (popup != NULL) {
    if (popup->visible && popup->tpw != NULL) {
      if (popup->on_paint) {
        popup->on_paint(popup, popup->payload);
      }
      wnoutrefresh(popup->tpw);
    }
    popup = popup->next;
  }
}

void gui_updateTPW(gui_Context *gui_context) {
  gui_context->refresh_tpw = true;
}

void gui_destroyToplevelPopup(gui_Context* gui_context, gui_TPW* popup) {
  if (!popup) {
    return;
  }

  // Remove from linked list
  gui_TPW* prev = NULL;
  gui_TPW* curr = gui_context->toplevel_popups;
  while (curr != NULL && curr != popup) {
    prev = curr;
    curr = curr->next;
  }

  if (curr == popup) {
    if (prev == NULL) {
      gui_context->toplevel_popups = popup->next;
    }
    else {
      prev->next = popup->next;
    }
  }

  bool had_focus = popup->has_focus;

  // Call destroy callback
  if (popup->on_destroy) {
    popup->on_destroy(popup, popup->payload);
  }

  // Destroy ncurses window
  if (popup->tpw) {
    delwin(popup->tpw);
  }

  free(popup);

  // Restore focus to the new top-most popup if needed
  if (had_focus && gui_context->toplevel_popups != NULL) {
    gui_context->toplevel_popups->has_focus = true;
  }

  gui_updateGUI(gui_context);
}

void gui_destroyAllToplevelPopups(gui_Context* gui_context) {
  while (gui_context->toplevel_popups != NULL) {
    gui_destroyToplevelPopup(gui_context, gui_context->toplevel_popups);
  }
}

void gui_setToplevelPopupFocus(gui_Context* gui_context, gui_TPW* popup) {
  gui_TPW* curr = gui_context->toplevel_popups;
  while (curr != NULL) {
    curr->has_focus = (curr == popup);
    curr = curr->next;
  }
}
