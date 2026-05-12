#include "editor_render.h"
#include <assert.h>
#include "../advanced/tree-sitter/tree_sitter_highlighter.h"
#include "../advanced/lsp/lsp_highlighter.h"
#include "../terminal/highlight.h"
#include "../terminal/term_handler.h"
void paint_gui(EditorContext* ctx) {
  FileContainer* fc = &ctx->files[ctx->current_file_index];
  // Refresh Editor Windows vars
  if (ctx->gui_context.edw_context.refresh_edw == true) {
    whd_reset(&ctx->highlight_descriptor);

    // calculate tree_sitter Highlight
    TS_highlightCurrentFile(&fc->highlight_data, ctx->gui_context.edw_context.ftw, fc->screen_x, fc->screen_y,
                            fc->cursor, &ctx->highlight_descriptor);

    // add lsp highlights
    LSP_highlightCurrentFile(&fc->lsp_datas, fc->cursor, &ctx->highlight_descriptor, &ctx->gui_context);
  }

  gui_repaintGUI(&ctx->gui_context, &ctx->highlight_descriptor, &ctx->pwd, ctx->files, ctx->file_count,
                 ctx->current_file_index);

  assert(checkFileIntegrity(fc->root) == true);
  assert(checkByteCountIntegrity(fc->root) == true);
}
