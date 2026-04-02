#include "lsp_handler.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


#include "../../environnement/global-variables.h"
#include "../../utils/tools.h"


void setLspDatas(LSP_Data* lsp_data, IO_FileID io_file) {
  bool did_lang_was_found = getLanguageStringIDForFile(lsp_data->lang_id, io_file);

  LSP_Server* lsp_server = NULL;
  if (did_lang_was_found == true) {
    lsp_server = getLSPServerForLanguage(&lsp_servers, lsp_data->lang_id);
  }
  lsp_data->is_enable = lsp_server != NULL;
  lsp_data->computed = NULL;
  strncpy(lsp_data->path_abs, io_file.path_abs, PATH_MAX - 1);

  if (lsp_data->is_enable) {
    lsp_data->computed = malloc(sizeof(LSP_ComputedData));
    assert(lsp_data->computed != NULL);
    LSP_initComputedData(lsp_data->computed);
  }
}

void destroyLspDatas(LSP_Data* lsp_datas) {
  LSP_destroyComputedData(lsp_datas->computed);
  free(lsp_datas->computed);
}

void LSP_destroyComputedData(LSP_ComputedData* lsp_payload) {
  if (!lsp_payload) {
    return;
  }

  // free diagnostics
  for (int i = 0; i < lsp_payload->diagnostics_size; i++) {
    LSP_destroyDiagnostic(lsp_payload->diagnostics + i);
  }
  free(lsp_payload->diagnostics);

  // free completionList
  LSP_destroyCompletionList(&lsp_payload->completions);

  // free hoverlist
  LSP_destroyHover(&lsp_payload->hover);

  // free definition-like lists
  LSP_destroyLocationArray(&lsp_payload->gotos);
}

void LSP_initComputedData(LSP_ComputedData* payload) {
  // TODO create LSP_init***** to avoid initializing every fields of every fields...

  // init diagnostics
  payload->diagnostics = NULL;
  payload->diagnostics_size = 0;

  // init completions
  payload->completions.completions.items = NULL;
  payload->completions.completions.size = 0;
  payload->completions.isIncomplete = false;

  // init hover
  payload->hover.size = 0;
  payload->hover.contents = NULL;
  payload->hover.is_range = false;

  // init definition-like lists
  payload->gotos.items = NULL;
  payload->gotos.size = 0;
  payload->goto_type = LSP_GOTO_DEFINITION;
}

void initLSPServerList(LSPServerLinkedList* list) { list->head = NULL; }

void destroyLSPServerList(LSPServerLinkedList* list) {
  LSPServerLinkedList_Cell* cell = list->head;
  while (cell != NULL) {
    LSPServerLinkedList_Cell* tmp = cell;
    cell = cell->next;

    LSP_closeLSPServer(&tmp->lsp_server);
    free(tmp);
  }
  list->head = NULL;
}

LSP_Server* allocateNewLSPServerToLSPServerList(LSPServerLinkedList* list) {
  LSPServerLinkedList_Cell* tmp = list->head;
  list->head = malloc(sizeof(LSPServerLinkedList_Cell));
  list->head->next = tmp;
  return &list->head->lsp_server;
}


void addLSPServerCellToLSPServerList(LSPServerLinkedList* list, LSPServerLinkedList_Cell* cell) {
  LSPServerLinkedList_Cell* tmp = list->head;
  list->head = cell;
  cell->next = tmp;
}

bool getProgName(char* language, char* prog_name, char* args) {
  // LSP_TOGGLE
  // return false;

  if (strcmp(language, "bash") == 0) {
    strcpy(prog_name, "bash-language-server");
    strcpy(args, "start");
  }
  else if (strcmp(language, "c") == 0) {
    strcpy(prog_name, "clangd");
    strcpy(args, "");
  }
  else if (strcmp(language, "python") == 0) {
    strcpy(prog_name, "pylsp");
    strcpy(args, "-v");
  }
  else if (strcmp(language, "markdown") == 0) {
    strcpy(prog_name, "marksman");
    strcpy(args, "");
  }
  else if (strcmp(language, "cpp") == 0) {
    strcpy(prog_name, "clangd");
    strcpy(args, "");
  }
  else {
    return false;
  }
  return true;
}

LSP_Server* getLSPServerForLanguage(LSPServerLinkedList* list, char* language) {
  LSPServerLinkedList_Cell* cell = list->head;

  while (cell != NULL && strcmp(cell->lsp_server.language, language) != 0) {
    cell = cell->next;
  }

  if (cell == NULL) {
    // If the cell is null, it will try to open the lsp_server associated.
    cell = malloc(sizeof(LSPServerLinkedList_Cell));
    bool load_result = loadNewLSPServer(&cell->lsp_server, language);

    // Open failed.
    if (load_result == false) {
      free(cell);
      return NULL;
    }

    // Open success. Adding the lsp_server to the current list.
    addLSPServerCellToLSPServerList(&lsp_servers, cell);
  }

  return &cell->lsp_server;
}

void* sendInit(void* args) {
  LSP_Server* container = args;
  LSP_initializeServer(container, "al", "0.5",
                       workspace_settings.is_used ? workspace_settings.dir_path : getenv("PWD"));
  return 0;
}

bool loadNewLSPServer(LSP_Server* container, char* language) {
  char prog_name[1000];
  char args[1000];

  if (getProgName(language, prog_name, args) == false) {
    return false;
  }

  bool opening_result = LSP_openLSPServer(prog_name, args, language, container);


  if (opening_result) {
    // execute the init in another thread to avoid blocking on the opening of a new thread.
    pthread_t t;
    pthread_create(&t, NULL, sendInit, container);
  }

  return opening_result;
}
