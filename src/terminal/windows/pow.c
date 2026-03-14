#include "pow.h"

#include <assert.h>
#include <string.h>
#include <unicode/urename.h>

#include "../../advanced/lsp/lsp_features/lsp_completion.h"
#include "../../io_management/viewport_history.h"
#include "../../utils/key_management.h"
#include "../term_handler.h"
#include "edw.h"


bool gui_resumeHoverInformation(Cursor* cursor, ViewPort* view_port, Hover* hover) {
  assert(hover->is_range);
  CursorDescriptor start_descriptor = positionToCursorDescriptor(hover->range.pos1);
  CursorDescriptor end_descriptor = positionToCursorDescriptor(hover->range.pos2);
  fprintf(stderr, "Last position saved (%d, %d), in range (%d, %d) -> (%d, %d)\n",
          view_port->gui->edw_context.lastMousePosition.row, view_port->gui->edw_context.lastMousePosition.column,
          start_descriptor.row, start_descriptor.column, end_descriptor.row, end_descriptor.column);


  // if the last cursor position is not inside the range we don't trigger the popup !
  if (!isCursorDescriptorBetweenOthers(view_port->gui->edw_context.lastMousePosition, start_descriptor,
                                       end_descriptor)) {
    fprintf(stderr, "don't opening popup !\n");
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
  return gui_showGenericPopup(view_port->gui, getAbsRow(cursor) - *view_port->screen_y + 1,
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

  gui_context->edw_context.completion_offset_y = 0;
  gui_context->edw_context.completion_selected = 0;

  wbkgd(gui_context->edw_context.pow, COLOR_PAIR(INFO_COLOR_PAIR));
}

void gui_setLastTextAnchor(GUIContext* gui_context, CursorDescriptor descriptor) {
  gui_context->edw_context.lastTextAnchor = descriptor;
}

void gui_showDiagnostic(GUIContext* gui_context, int y, int x, Diagnostic* diagnostic) {
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
    case LSP_ERROR:
      color = ERROR_COLOR_PAIR;
      break;
    case LSP_WARNING:
      color = WARNING_COLOR_PAIR;
      break;
    case LSP_INFORMATION:
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
  int width = getmaxx(context->pow), height = getmaxy(context->pow);
  werase(context->pow);

  // for the height lines.
  for (int i = 0; i < height; i++) {
    int index = context->completion_offset_y + i;
    if (index >= lsp_data->completions.completions.size) {
      break; // reach the end of the completions.
    }

    // setup color for current line.
    if (context->completion_selected == index) {
      wattr_set(context->pow, A_NORMAL | A_ITALIC, WARNING_COLOR_HOVER_PAIR, NULL);
    }
    else {
      wattr_set(context->pow, A_NORMAL | A_ITALIC, INFO_COLOR_PAIR, NULL);
    }

    printToWindow(context->pow, trim(lsp_data->completions.completions.items[index].label), -1, 0, i, width, 1);
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
    default:
      break;
  }
}


bool gui_handleCompletionInput(GUIContext* context, Cursor* cursor, int c_hash, int c_raw, LSP_ComputedData* lsp_data,
                               History** history_p, PayloadStateChange payload_state_change) {
  int height = getmaxy(context->edw_context.pow);
  switch (c_hash) {
    case H_KEY_UP:
      if (context->edw_context.completion_selected > 0) {
        context->edw_context.completion_selected--;
      }
      if (context->edw_context.completion_selected < context->edw_context.completion_offset_y) {
        context->edw_context.completion_offset_y = context->edw_context.completion_selected;
      }
      return true;
    case H_KEY_DOWN:
      context->edw_context.completion_selected++;
      if (context->edw_context.completion_selected >= lsp_data->completions.completions.size) {
        context->edw_context.completion_selected = lsp_data->completions.completions.size - 1;
      }
      if (context->edw_context.completion_selected >= context->edw_context.completion_offset_y + height) {
        context->edw_context.completion_offset_y++;
      }
      return true;
    case '\n':
    case KEY_ENTER:
      if (context->edw_context.completion_selected < lsp_data->completions.completions.size) {
        LSP_executeCompletion(cursor,
                              lsp_data->completions.completions.items + context->edw_context.completion_selected,
                              history_p, payload_state_change);
      }
      gui_closePopup(context);
      return true;
    case KEY_TAB:
      if (context->edw_context.completion_selected < lsp_data->completions.completions.size) {
        LSP_executeCompletion(cursor,
                              lsp_data->completions.completions.items + context->edw_context.completion_selected,
                              history_p, payload_state_change);
      }
      gui_closePopup(context);
      return true;
    default:
      break;
  }

  return false;
}

bool gui_handlePopupInput(GUIContext* context, Cursor* cursor, int c_hash, int c_raw, LSP_ComputedData* lsp_data,
                          History** history_p, PayloadStateChange payload_state_change) {
  if (context->edw_context.show_pow == false || context->edw_context.pow == NULL) {
    return false;
  }
  switch (context->edw_context.pow_owner) {
    case COMPLETION:
      return gui_handleCompletionInput(context, cursor, c_hash, c_raw, lsp_data, history_p, payload_state_change);
    default:
      break;
  }
  return false;
}
