#include "pow.h"

#include <string.h>

#include "../term_handler.h"
#include "edw.h"


void gui_showDiagnostic(GUIContext* gui_context, int y, int x, Diagnostic* diagnostic) {
  if (diagnostic == NULL) {
    return;
  }

  int height = 3;
  int unused = 0;
  int max_width = getmaxx(gui_context->edw_context.ftw) - 2;

  int ch_length = strlen(diagnostic->message);
  countStringFrame(diagnostic->message, ch_length, &height, &unused, &max_width);

  fprintf(stderr, "Popup settings:  height = %d, max_width = %d\n", height, max_width);

  bool isOpened =
    gui_showPopup(gui_context, y + height, x - 2, height, min(getmaxx(gui_context->edw_context.ftw), max_width + 2));
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

  wattr_set(gui_context->edw_context.pow, A_NORMAL, color, NULL);

  box(gui_context->edw_context.pow, 0, 0);

  printToWindow(gui_context->edw_context.pow, diagnostic->message, ch_length, 1, 1, max_width);
}
