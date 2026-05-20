#include <assert.h>
#include <ncurses.h>
#include <stdio.h>

#include "advanced/lsp/lsp_dispatcher.h"
#include "advanced/tree-sitter/tree_manager.h"
#include "config/config.h"
#include "config/language_feature.h"
#include "data-management/file_management.h"
#include "environnement/global_variables.h"
#include "io-management/workspace_settings.h"
#include "terminal/highlight.h"
#include "terminal/term_handler.h"
#include "utils/key_management.h"

#include "core/editor_context.h"
#include "core/editor_destroy.h"
#include "core/editor_init.h"
#include "core/editor_input.h"
#include "core/editor_lsp.h"
#include "core/editor_render.h"
#include "core/editor_state.h"
#include "environnement/setup.h"

// Global vars.
int color_pair = 5;
int color_index = 20;
cJSON* config;
ParserList parsers;
LSPServerLinkedList lsp_servers;
WorkspaceSettings workspace_settings;
ft_LanguageFeatureList language_features;


int main(int file_count, char** args) {
  /// --- Setup Program ---
  setupProgramEnvironnemnt();

  char** file_names = args;
  file_names++;
  file_count--;

  /// --- Init global vars ---
  // Load config
  config = loadConfig();
  // Language features
  ft_loadLanguageFeatures();
  // Parser Datas
  initParserList(&parsers);
  // LSP Datas
  initLSPServerList(&lsp_servers);

  /// --- Initiate EditorContext ---
  EditorContext ctx;
  initDefaultContext(file_count, &ctx);
  // init context structs
  whd_init(&ctx.highlight_descriptor);

  /// --- Init TUI ---
  // Init GUIContext
  gui_initGUIContext(&ctx.gui_context);
  // Init terminal with ncurses
  gui_initNCurses(&ctx.gui_context);

  /// --- Setup Workspace Settings ---
  setupWorkspace(&workspace_settings, &ctx.file_count, &file_names, &ctx.gui_context, &ctx.current_file_index);

  /// --- Setup Opened Files ---
  setupOpenedFiles(&ctx.file_count, file_names, &ctx.files);

  /// --- Setup File Explorer ---
  setupFileExplorer(&ctx);

  /// --- Main Loop ---
  while (true) {
    // automated behaviors
    runPostProcessing(&ctx);
    // update GUI
    runGuiUpdate(&ctx);

    // read from input stream
  read_input:;
    int c, hash;
    readNextInput(&ctx, &c, &hash);

    //// ---- BEGIN background / delayed operations BLOCK ----

    // handle lsp servers
    handleLspServers(&ctx, &c, &hash);

    // if lsp ask to refresh local_vars we have to execute post processing
    if (ctx.refresh_local_vars) {
      // !! WARNING !!: careful we may drop an input doing this jump. We consider this drop non important.
      continue;
    }

    //// ---- END background / delayed operations BLOCK ----

    // if input is empty, execute nothing and read again
    if (hash == ERR) {
      goto read_input;
    }

    // if input is mouse, fetch and detect mouse event
    if (hash == KEY_MOUSE) {
      if (getmouse(&ctx.m_event) != OK) {
        fprintf(stderr, "MOUVE_EVENT_NOT_OK !\r\n");
        goto read_input;
      }
      detectComplexMouseEvents(&ctx.m_event);
    }

    // setup default loop end behavior
    EventLoopAction loopEnd = EVENT_READ_INPUT;

    // first dispatch input to popup.
    bool has_popup_handle_input = handlePopupInput(&ctx, c, hash);

    // if popup hasn't consumed the input execute global key handling
    if (!has_popup_handle_input) {
      loopEnd = runKeyHandler(&ctx, c, hash);
    }


    // process the loop end behavior
    switch (loopEnd) {
      case EVENT_QUIT:
        goto end;
      case EVENT_READ_INPUT:
        if (gui_doesGUINeedRepaint(&ctx.gui_context)) {
          continue;
        }
        goto read_input;
      case EVENT_CONTINUE:
        continue;
      default:
        assert(false);
    }
  }

  /// --- Editor Destroy ---
end:;
  // release ressources
  finalizeEditor(&ctx);

  return 0;
}
