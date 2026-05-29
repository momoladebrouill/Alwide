#include "lsp_highlighter.h"

#include <assert.h>
#include <ncurses.h>

#include "../../environnement/global_variables.h"
#include "../../terminal/highlight.h"
#include "../../terminal/windows/gui_entities.h"
#include "lsp_handler.h"

void LSP_highlightCurrentFile(LSP_Data* lsp_datas, Cursor cursor, WindowHighlightDescriptor* highlight_descriptor,
                              gui_Context* context) {
  if (!lsp_datas->is_enable) {
    return;
  }
  assert(lsp_datas->computed != NULL);
  for (int i = 0; i < lsp_datas->computed->diagnostics.size; i++) {
    LSP_Diagnostic* d = &lsp_datas->computed->diagnostics.items[i];
    LSP_Server* lsp = getLSPServerForLanguage(&lsp_servers, lsp_datas->lang_id);
    Cursor begin_cursor = LSP_tryToReachCursorForLSPPosition(lsp, cursor, d->range.pos1);
    begin_cursor = moveRight(begin_cursor);
    Cursor end_cursor = LSP_tryToReachCursorForLSPPosition(lsp, cursor, d->range.pos2);

    attr_t attr = A_UNDERLINE;
    NCURSES_PAIRS_T color = COLOR_RED;

    whd_insertDescriptor(highlight_descriptor, begin_cursor, end_cursor, color, attr, 0, false, (LineMarker)d->severity,
                         d);
  }

  // lsp_datas->computed->hover.
  if (lsp_datas->computed->hover.is_range && lsp_datas->computed->hover.size != 0 &&
      context->edw_context.pow_owner == HOVER_DIAGNOSTICS && context->edw_context.show_pow) {
    LSP_Server* lsp = getLSPServerForLanguage(&lsp_servers, lsp_datas->lang_id);

    Cursor begin_cursor = LSP_tryToReachCursorForLSPPosition(lsp, cursor, lsp_datas->computed->hover.range.pos1);
    begin_cursor = moveRight(begin_cursor);
    Cursor end_cursor = LSP_tryToReachCursorForLSPPosition(lsp, cursor, lsp_datas->computed->hover.range.pos2);

    attr_t attr = A_UNDERLINE;
    NCURSES_PAIRS_T color = DEFAULT_COLOR_PAIR;

    whd_insertDescriptor(highlight_descriptor, begin_cursor, end_cursor, color, attr, 100, false, LINE_MARKER_NONE,
                         NULL);
  }
  // TODO highlight with marker
}
