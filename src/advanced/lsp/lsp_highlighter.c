#include "lsp_highlighter.h"

#include <assert.h>
#include <ncurses.h>

#include "../../terminal/highlight.h"
#include "../../terminal/windows/gui_entities.h"
#include "lsp_handler.h"

void LSP_highlightCurrentFile(LSP_Data* lsp_datas, Cursor cursor, WindowHighlightDescriptor* highlight_descriptor,
                              GUIContext* context) {
  if (!lsp_datas->is_enable) {
    return;
  }
  assert(lsp_datas->computed != NULL);
  for (int i = 0; i < lsp_datas->computed->diagnostics_size; i++) {
    Cursor begin_cursor = tryToReachAbsPosition(cursor, lsp_datas->computed->diagnostics[i].range.pos1.row + 1,
                                                lsp_datas->computed->diagnostics[i].range.pos1.column + 1);
    Cursor end_cursor = tryToReachAbsPosition(cursor, lsp_datas->computed->diagnostics[i].range.pos2.row + 1,
                                              lsp_datas->computed->diagnostics[i].range.pos2.column);

    attr_t attr = A_UNDERLINE;
    NCURSES_PAIRS_T color = COLOR_RED;

    whd_insertDescriptor(highlight_descriptor, begin_cursor, end_cursor, color, attr, 0, false,
                         (LineMarker)lsp_datas->computed->diagnostics[i].severity,
                         lsp_datas->computed->diagnostics + i);
  }

  // lsp_datas->computed->hover.
  if (lsp_datas->computed->hover.is_range && lsp_datas->computed->hover.size != 0 &&
      context->edw_context.pow_owner == HOVER_DIAGNOSTICS && context->edw_context.show_pow) {
    CursorDescriptor begin_descriptor = positionToCursorDescriptor(lsp_datas->computed->hover.range.pos1);
    CursorDescriptor end_descriptor = positionToCursorDescriptor(lsp_datas->computed->hover.range.pos2);

    Cursor begin_cursor = tryToReachAbsPosition(cursor, begin_descriptor.row, begin_descriptor.column + 1);
    Cursor end_cursor = tryToReachAbsPosition(cursor, end_descriptor.row, end_descriptor.column);

    attr_t attr = A_UNDERLINE;
    NCURSES_PAIRS_T color = 0;

    whd_insertDescriptor(highlight_descriptor, begin_cursor, end_cursor, color, attr, 100, false, LINE_MARKER_NONE,
                         NULL);
  }
  // TODO highlight with marker
}
