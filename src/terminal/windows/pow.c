#include "pow.h"

#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "../../advanced/lsp/lsp_features/lsp_completion.h"
#include "../../advanced/lsp/lsp_features/lsp_goto.h"
#include "../../io_management/viewport_history.h"
#include "../../utils/key_management.h"
#include "../click_handler.h"
#include "../term_handler.h"
#include "edw.h"


bool gui_resumeHoverInformation(Cursor* cursor, ViewPort* view_port, LSP_Hover* hover) {
  assert(hover->is_range);
  CursorDescriptor start_descriptor = positionToCursorDescriptor(hover->range.pos1);
  CursorDescriptor end_descriptor = positionToCursorDescriptor(hover->range.pos2);
  fprintf(stderr, "Last position saved (%d, %d), in range (%d, %d) -> (%d, %d)\n",
          view_port->gui->edw_context.lastMousePosition.row, view_port->gui->edw_context.lastMousePosition.column,
          start_descriptor.row, start_descriptor.column, end_descriptor.row, end_descriptor.column);


  // if the last cursor position is not inside the range we don't trigger the popup ! Or if the popup is owned by
  // GOTO_CHOICE
  if (!cursor_desc_is_between(view_port->gui->edw_context.lastMousePosition, start_descriptor, end_descriptor) ||
      view_port->gui->edw_context.pow_owner == GOTO_CHOICE) {
    return false;
  }

  fprintf(stderr, "opening popup !\n");
  Cursor new_cur = tryToReachAbsPosition(*cursor, start_descriptor.row, start_descriptor.column);

  // calculating the best popup size.
  int total_height = 0;
  int global_max_width = 0;
  int max_popup_width = 10000;
  for (int i = 0; i < hover->size; i++) {
    int rows = 0;
    int unused_cols = 0;
    int screen_max_width = max_popup_width - 2;
    countStringFrame(hover->contents[i].value, strlen(hover->contents[i].value), &rows, &unused_cols,
                     &screen_max_width);
    total_height += (rows + 1);
    if (screen_max_width > global_max_width) {
      global_max_width = screen_max_width;
    }
    if (i < hover->size - 1) {
      total_height++; // separator
    }
  }

  gui_showGenericPopupWithTextAnchor(view_port, &new_cur, total_height + 2, global_max_width + 2, HOVER_DIAGNOSTICS);
  return true;
}

bool gui_resumeGotoChoice(ViewPort* view_port, Cursor* cursor) {
  gui_showGenericPopupWithTextAnchor(view_port, cursor, 8, 60, GOTO_CHOICE);
  return true;
}

bool gui_resumeCompletionTextAnchor(ViewPort* view_port, Cursor* cursor) {
  if ((cursor->file_id.absolute_row == view_port->gui->edw_context.lastTextAnchor.row &&
       cursor->line_id.absolute_column == view_port->gui->edw_context.lastTextAnchor.column &&
       view_port->gui->edw_context.pow_owner == NO_OWNER) ||
      view_port->gui->edw_context.pow_owner == COMPLETION) {
    gui_showGenericPopupWithTextAnchor(view_port, cursor, 7, 50, COMPLETION);
    return true;
  }
  return false;
}

void gui_showGenericPopupWithTextAnchor(ViewPort* view_port, Cursor* cursor, int height, int width,
                                        PopupOwner popup_owner) {
  return gui_showGenericPopup(view_port->gui, cursor_row(*cursor) - *view_port->screen_y + 1,
                              getScreenXForCursor(*cursor, *view_port->screen_x), height, width, popup_owner);
}

void gui_showGenericPopup(GUIContext* gui_context, int y, int x, int prefered_height, int prefered_width,
                          PopupOwner popup_owner) {
  int height = min(getmaxy(gui_context->edw_context.ftw) - y, prefered_height);
  int width = min(getmaxx(gui_context->edw_context.ftw) - x, prefered_width);

  bool isOpened = gui_showPopup(gui_context, y + height, x - 2, height, width, popup_owner);
  // couldn't show popup
  if (!isOpened) {
    return;
  }

  gui_context->edw_context.item_select_offset_y = 0;
  gui_context->edw_context.item_selected = 0;

  wbkgd(gui_context->edw_context.pow, COLOR_PAIR(INFO_COLOR_PAIR));
}

void gui_setLastTextAnchor(GUIContext* gui_context, CursorDescriptor descriptor) {
  gui_context->edw_context.lastTextAnchor = descriptor;
}

void gui_showDiagnostic(GUIContext* gui_context, int y, int x, LSP_Diagnostic* diagnostic) {
  if (diagnostic == NULL) {
    return;
  }

  int height = 3;
  int unused = 0;
  int max_width = getmaxx(gui_context->edw_context.ftw) - 2;

  int ch_length = strlen(diagnostic->message);
  countStringFrame(diagnostic->message, ch_length, &height, &unused, &max_width);

  bool isOpened = gui_showPopup(gui_context, y + height, x - 2, height,
                                min(getmaxx(gui_context->edw_context.ftw), max_width + 2), DIAGNOSTICS);
  // couldn't show popup
  if (!isOpened) {
    return;
  }

  int color = 0;
  switch (diagnostic->severity) {
    case LSP_DIAG_ERROR:
      color = ERROR_COLOR_PAIR;
      break;
    case LSP_DIAG_WARNING:
      color = WARNING_COLOR_PAIR;
      break;
    case LSP_DIAG_INFORMATION:
      color = INFO_COLOR_PAIR;
      break;
    default:
      color = 0;
      break;
  }
  wbkgd(gui_context->edw_context.pow, COLOR_PAIR(color));
  wattr_set(gui_context->edw_context.pow, A_NORMAL, color, NULL);

  box(gui_context->edw_context.pow, 0, 0);

  printToWindow(gui_context->edw_context.pow, diagnostic->message, ch_length, 1, 1, max_width, 0);
}

void gui_printCompletionPopup(EDW_GUIContext* context, Cursor* cursor, LSP_ComputedData* lsp_data) {
  if (lsp_data == NULL) {
    fprintf(stderr, "INTERNAL ERROR : Asked to print the popup on a non lsp file.\n");
    return;
  }
  int width = getmaxx(context->pow), height = getmaxy(context->pow);
  werase(context->pow);

  // for the height lines.
  for (int i = 0; i < height; i++) {
    int index = context->item_select_offset_y + i;
    if (index >= lsp_data->completions.completions.size) {
      break; // reach the end of the completions.
    }

    // setup color for current line.
    if (context->item_selected == index) {
      wattr_set(context->pow, A_NORMAL | A_ITALIC, WARNING_COLOR_HOVER_PAIR, NULL);
    }
    else {
      wattr_set(context->pow, A_NORMAL | A_ITALIC, INFO_COLOR_PAIR, NULL);
    }

    printToWindow(context->pow, trim(lsp_data->completions.completions.items[index].label), -1, 0, i, width, 1);
  }
}

const char* gui_getGotoLabel(LSP_GOTO_TYPE type) {
  switch (type) {
    case LSP_GOTO_DEFINITION:
      return "Definitions";
    case LSP_GOTO_DECLARATION:
      return "Declarations";
    case LSP_GOTO_TYPE_DEFINITION:
      return "Type Definitions";
    case LSP_GOTO_IMPLEMENTATION:
      return "Implementations";
    case LSP_FIND_REFERENCE:
      return "References";
    default:
      return "Choices";
  }
}

void gui_printGotoChoicePopup(EDW_GUIContext* context, Cursor* cursor, LSP_ComputedData* lsp_data) {
  int width = getmaxx(context->pow), height = getmaxy(context->pow);
  werase(context->pow);

  wattr_set(context->pow, A_BOLD, INFO_COLOR_PAIR, NULL);

  box(context->pow, 0, 0);
  mvwprintw(context->pow, 0, 1, " Multiple %s found ", gui_getGotoLabel(lsp_data->goto_type));

  for (int i = 0; i < height - 2; i++) {
    int index = context->item_select_offset_y + i;
    if (index >= lsp_data->gotos.size) {
      break;
    }

    if (context->item_selected == index) {
      wattr_set(context->pow, A_NORMAL | A_REVERSE, WARNING_COLOR_HOVER_PAIR, NULL);
    }
    else {
      wattr_set(context->pow, A_NORMAL, INFO_COLOR_PAIR, NULL);
    }

    LSP_Location loc = lsp_data->gotos.items[index];
    char line[PATH_MAX + 30];
    char* filename_copy = strdup(loc.file_name.file_name);
    snprintf(line, sizeof(line), "  📄 %-20s : line %-4d", basename(filename_copy), loc.range.pos1.row + 1);
    free(filename_copy);

    printToWindow(context->pow, line, -1, 1, i + 1, width - 2, 1);
  }
}

void gui_printHoverPopup(EDW_GUIContext* context, Cursor* cursor, LSP_ComputedData* lsp_data) {
  int width = getmaxx(context->pow), height = getmaxy(context->pow);
  werase(context->pow);
  wattr_set(context->pow, A_NORMAL, INFO_COLOR_PAIR, NULL);

  box(context->pow, 0, 0);

  int current_y = 1;
  // Hover Information
  for (int i = 0; i < lsp_data->hover.size; i++) {
    if (current_y >= height) {
      break;
    }
    char* text = lsp_data->hover.contents[i].value;
    int len = strlen(text);

    printToWindow(context->pow, text, len, 1, current_y, width - 2, height - current_y - 1);

    int lines_consumed = 0;
    int current_column = 0;
    int dummy_width = width;
    countStringFrame(text, len, &lines_consumed, &current_column, &dummy_width);

    current_y += (lines_consumed + 1);

    // Add a separator if there are more items
    if (i < lsp_data->hover.size - 1 && current_y < height) {
      mvwhline(context->pow, current_y, 0, ACS_HLINE, width);
      current_y++;
    }
  }
}


void gui_printPopup(EDW_GUIContext* context, Cursor* cursor, LSP_ComputedData* lsp_data) {
  switch (context->pow_owner) {
    case COMPLETION:
      gui_printCompletionPopup(context, cursor, lsp_data);
      break;
    case HOVER_DIAGNOSTICS:
      gui_printHoverPopup(context, cursor, lsp_data);
      break;
    case GOTO_CHOICE:
      gui_printGotoChoicePopup(context, cursor, lsp_data);
      break;
    default:
      break;
  }
}

bool gui_handleCompletionInput(GUIContext* context, Cursor* cursor, int c_hash, int c_raw, LSP_ComputedData* lsp_data,
                               History** history_p, PayloadStateChange payload_state_change, MEVENT* m_event) {
  int height = getmaxy(context->edw_context.pow);

  if (c_hash == KEY_MOUSE && m_event != NULL) {

    int item_count = lsp_data->completions.completions.size;
    if (m_event->bstate & BUTTON4_PRESSED) {
      if (context->edw_context.item_select_offset_y > 0) {
        context->edw_context.item_select_offset_y--;
        gui_updateEDW(context);
      }
    }
    else if (m_event->bstate & BUTTON5_PRESSED) {
      if (context->edw_context.item_select_offset_y + height < item_count) {
        context->edw_context.item_select_offset_y++;
        gui_updateEDW(context);
      }
    }
    else if (m_event->bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) {
      int clicked_item = context->edw_context.item_select_offset_y + (m_event->y - getbegy(context->edw_context.pow));
      if (clicked_item < item_count) {
        context->edw_context.item_selected = clicked_item;
        gui_updateEDW(context);
      }
    }

    if (m_event->bstate & BUTTON1_DOUBLE_CLICKED) {
      int clicked_item = context->edw_context.item_select_offset_y + (m_event->y - getbegy(context->edw_context.pow));
      if (clicked_item < item_count) {
        LSP_executeCompletion(cursor, lsp_data->completions.completions.items + clicked_item, history_p,
                              payload_state_change);
        gui_closePopup(context);
      }
    }

    return true;
  }

  switch (c_hash) {
    case H_KEY_UP:
      if (context->edw_context.item_selected > 0) {
        context->edw_context.item_selected--;
      }
      if (context->edw_context.item_selected < context->edw_context.item_select_offset_y) {
        context->edw_context.item_select_offset_y = context->edw_context.item_selected;
      }
      return true;
    case H_KEY_DOWN:
      context->edw_context.item_selected++;
      if (context->edw_context.item_selected >= lsp_data->completions.completions.size) {
        context->edw_context.item_selected = lsp_data->completions.completions.size - 1;
      }
      if (context->edw_context.item_selected >= context->edw_context.item_select_offset_y + height) {
        context->edw_context.item_select_offset_y++;
      }
      return true;
    case '\n':
    case KEY_ENTER:
      if (context->edw_context.item_selected < lsp_data->completions.completions.size) {
        LSP_executeCompletion(cursor, lsp_data->completions.completions.items + context->edw_context.item_selected,
                              history_p, payload_state_change);
      }
      gui_closePopup(context);
      return true;
    case KEY_TAB:
      if (context->edw_context.item_selected < lsp_data->completions.completions.size) {
        LSP_executeCompletion(cursor, lsp_data->completions.completions.items + context->edw_context.item_selected,
                              history_p, payload_state_change);
      }
      gui_closePopup(context);
      return true;
    default:
      break;
  }

  return false;
}

bool gui_handleGotoChoiceInput(GUIContext* context, Cursor* cursor, int c_hash, int c_raw, LSP_ComputedData* lsp_data,
                               History** history_p, PayloadStateChange payload_state_change, DispatcherPayload* payload,
                               MEVENT* m_event) {
  int height = getmaxy(context->edw_context.pow) - 2;

  if (c_hash == KEY_MOUSE && m_event != NULL) {
    int item_count = lsp_data->gotos.size;
    int items_per_page = height;

    if (m_event->bstate & BUTTON4_PRESSED) {
      if (context->edw_context.item_select_offset_y > 0) {
        context->edw_context.item_select_offset_y--;
        gui_updateEDW(context);
      }
    }
    else if (m_event->bstate & BUTTON5_PRESSED) {
      if (context->edw_context.item_select_offset_y + items_per_page < item_count) {
        context->edw_context.item_select_offset_y++;
        gui_updateEDW(context);
      }
    }
    else if (m_event->bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) {
      int clicked_item =
        context->edw_context.item_select_offset_y + (m_event->y - getbegy(context->edw_context.pow) - 1);
      if (clicked_item >= 0 && clicked_item < item_count) {
        context->edw_context.item_selected = clicked_item;
        gui_updateEDW(context);
      }
    }

    if (m_event->bstate & BUTTON1_DOUBLE_CLICKED) {
      int clicked_item =
        context->edw_context.item_select_offset_y + (m_event->y - getbegy(context->edw_context.pow) - 1);
      if (clicked_item >= 0 && clicked_item < item_count) {
        jumpToLocation(payload, lsp_data->gotos.items[clicked_item]);
        gui_closePopup(context);
      }
    }
    return true;

  }

  switch (c_hash) {
    case H_KEY_UP:
      if (context->edw_context.item_selected > 0) {
        context->edw_context.item_selected--;
      }
      if (context->edw_context.item_selected < context->edw_context.item_select_offset_y) {
        context->edw_context.item_select_offset_y = context->edw_context.item_selected;
      }
      return true;
    case H_KEY_DOWN:
      context->edw_context.item_selected++;
      if (context->edw_context.item_selected >= lsp_data->gotos.size) {
        context->edw_context.item_selected = lsp_data->gotos.size - 1;
      }
      if (context->edw_context.item_selected >= context->edw_context.item_select_offset_y + height) {
        context->edw_context.item_select_offset_y++;
      }
      return true;
    case '\n':
    case KEY_ENTER:
    case KEY_TAB:
      if (context->edw_context.item_selected < lsp_data->gotos.size && payload != NULL) {
        jumpToLocation(payload, lsp_data->gotos.items[context->edw_context.item_selected]);
      }
      gui_closePopup(context);
      return true;
    default:
      break;
  }

  return false;
}

bool gui_handlePopupInput(GUIContext* context, Cursor* cursor, int c_hash, int c_raw, LSP_ComputedData* lsp_data,
                          History** history_p, PayloadStateChange payload_state_change, DispatcherPayload* payload,
                          MEVENT* m_event) {
  if (context->edw_context.show_pow == false || context->edw_context.pow == NULL) {
    return false;
  }

  if (c_hash == KEY_MOUSE && m_event != NULL && !isClickInsideWindow(context->edw_context.pow, m_event)) {
    return false;
  }

  switch (context->edw_context.pow_owner) {
    case COMPLETION:
      return gui_handleCompletionInput(context, cursor, c_hash, c_raw, lsp_data, history_p, payload_state_change,
                                       m_event);
    case GOTO_CHOICE:
      return gui_handleGotoChoiceInput(context, cursor, c_hash, c_raw, lsp_data, history_p, payload_state_change,
                                       payload, m_event);
    default:
      break;
  }
  return false;
}
