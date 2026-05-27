#include "language_popup.h"
#include <stdlib.h>
#include <string.h>
#include <sys/ttydefaults.h>

#include "../../../../lib/tree-sitter/lib/include/tree_sitter/api.h"
#include "../../../advanced/lsp/lsp_handler.h"
#include "../../../advanced/tree-sitter/tree_manager.h"
#include "../../../core/editor_context.h"
#include "../../../environnement/constants.h"
#include "../../../environnement/global_variables.h"
#include "../../../utils/key_management.h"
#include "../../click_handler.h"
#include "../../term_handler.h"
#include "../tpw.h"

typedef struct {
  EditorContext* ctx;
  int selected_index;
} LanguageSelectPopupContext;

static void paint_lang_popup(gui_TPW* popup, void* payload) {
  LanguageSelectPopupContext* state = (LanguageSelectPopupContext*)payload;
  WINDOW* w = popup->tpw;
  int width = popup->width;

  werase(w);
  box(w, 0, 0);

  // Sleek Title
  wattron(w, A_BOLD | COLOR_PAIR(INFO_COLOR_PAIR));
  mvwprintw(w, 0, (width - 15) / 2, " [ Languages ] ");
  wattroff(w, A_BOLD | COLOR_PAIR(INFO_COLOR_PAIR));

  // List all language features
  for (int i = 0; i < language_features.size; i++) {
    bool is_selected = (i == state->selected_index);
    if (is_selected) {
      wattron(w, A_REVERSE | A_BOLD | COLOR_PAIR(INFO_COLOR_PAIR));
    }
    // Print label padded to fill the row beautifully
    mvwprintw(w, i + 1, 2, "%-21s", language_features.list[i].label);
    if (is_selected) {
      wattroff(w, A_REVERSE | A_BOLD | COLOR_PAIR(INFO_COLOR_PAIR));
    }
  }
}

static void apply_language_change(EditorContext* ctx, int index) {
  if (index < 0 || index >= language_features.size) {
    return;
  }

  FileContainer* fc = &ctx->files[ctx->current_file_index];
  LF_LanguageFeature* new_feature = &language_features.list[index];

  if (fc->feature == new_feature) {
    return; // No change
  }

  // Free/Clean old LSP/Highlight data
  if (fc->highlight_data.tree != NULL) {
    ts_tree_delete(fc->highlight_data.tree);
    fc->highlight_data.tree = NULL;
  }
  destroyLspDatas(&fc->lsp_datas);
  fc->lsp_datas.computed = NULL; // prevent double free later

  // Set the new feature
  fc->feature = new_feature;

  // Initialize highlighting/LSP with new feature
  setFileHighlightDatas(&fc->highlight_data, fc->feature);
  setLspDatas(&fc->lsp_datas, fc->io_file, fc->feature);

  // Refresh the editor and status bar
  ctx->gui_context.edw_context.refresh_edw = true;
  ctx->refresh_local_vars = true;
  gui_updateGUI(&ctx->gui_context);
}

static bool input_lang_popup(gui_TPW* popup, int c_raw, int c_hash, MEVENT* m_event, void* payload) {
  LanguageSelectPopupContext* state = payload;
  EditorContext* ctx = state->ctx;

  if (c_hash == H_KEY_ESCAPE || c_raw == CTRL('[')) {
    gui_destroyToplevelPopup(&ctx->gui_context, popup);
    return true;
  }

  if (c_hash == H_KEY_UP || c_raw == KEY_UP) {
    if (state->selected_index > 0) {
      state->selected_index--;
    }
    else {
      state->selected_index = language_features.size - 1;
    }
    gui_updateTPW(&ctx->gui_context);
    return true;
  }

  if (c_hash == H_KEY_DOWN || c_raw == KEY_DOWN) {
    if (state->selected_index < language_features.size - 1) {
      state->selected_index++;
    }
    else {
      state->selected_index = 0;
    }
    gui_updateTPW(&ctx->gui_context);
    return true;
  }

  if (c_raw == KEY_ENTER || c_raw == '\n' || c_raw == '\r') {
    apply_language_change(ctx, state->selected_index);
    gui_destroyToplevelPopup(&ctx->gui_context, popup);
    return true;
  }

  if (c_raw == KEY_MOUSE && (m_event->bstate & BUTTON1_CLICKED || m_event->bstate & BUTTON1_PRESSED)) {
    int clicked_y = m_event->y - getbegy(popup->tpw);
    int index = clicked_y - 1;
    if (index >= 0 && index < language_features.size) {
      state->selected_index = index;
      return true;
    }
    gui_updateTPW(&ctx->gui_context);
  }

  if (c_raw == KEY_MOUSE && m_event->bstate & BUTTON1_DOUBLE_CLICKED) {
    int clicked_y = m_event->y - getbegy(popup->tpw);
    int index = clicked_y - 1;
    if (index >= 0 && index < language_features.size) {
      apply_language_change(ctx, index);
      gui_destroyToplevelPopup(&ctx->gui_context, popup);
      return true;
    }
  }
}

static void destroy_lang_popup(gui_TPW* popup, void* payload) {
  LanguageSelectPopupContext* state = payload;
  if (state) {
    free(state);
  }
}

void gui_openLanguageSelectPopup(void* ctx_void) {
  EditorContext* ctx = ctx_void;
  LanguageSelectPopupContext* state = malloc(sizeof(LanguageSelectPopupContext));
  if (!state) {
    return;
  }

  state->ctx = ctx;
  state->selected_index = 0;

  // Find currently selected language
  FileContainer* fc = &ctx->files[ctx->current_file_index];
  for (int i = 0; i < language_features.size; i++) {
    if (fc->feature == &language_features.list[i]) {
      state->selected_index = i;
      break;
    }
  }

  int popup_w = 25;
  int popup_h = language_features.size + 2;
  int popup_y = LINES - 1 - popup_h;
  int popup_x = COLS - popup_w;

  gui_TPW* popup = gui_createToplevelPopup(&ctx->gui_context, popup_y, popup_x, popup_h, popup_w, paint_lang_popup,
                                           input_lang_popup, destroy_lang_popup, state);
  gui_setTPWStrongFocus(&ctx->gui_context, popup, true);
  gui_updateGUI(&ctx->gui_context);
}
