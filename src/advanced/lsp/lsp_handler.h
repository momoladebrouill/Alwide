#ifndef LSP_HANDLER_H
#define LSP_HANDLER_H
#include <stdbool.h>

#include "../../environnement/constants.h"
#include "../../io_management/io_manager.h"
#include "lsp_client.h"


typedef struct {
  LSP_Diagnostic* diagnostics;
  int diagnostics_size;
  LSP_CompletionList completions;
  LSP_Hover hover;
  // used by the TUI when multiple choice happens to let choose the user.
  LSP_LocationArray gotos;
  LSP_GOTO_TYPE goto_type;
} LSP_ComputedData;

void LSP_initComputedData(LSP_ComputedData* payload);


typedef struct {
  char lang_id[LANG_ID_LENGTH];
  bool is_enable;
  LSP_ComputedData* computed;
  char path_abs[PATH_MAX];
} LSP_Data;

struct _LSPServerLinkedList_Cell {
  LSP_Server lsp_server;
  struct _LSPServerLinkedList_Cell* next;
};

typedef struct _LSPServerLinkedList_Cell LSPServerLinkedList_Cell;


typedef struct {
  LSPServerLinkedList_Cell* head;
} LSPServerLinkedList;


void setLspDatas(LSP_Data* lsp_data, IO_FileID io_file);

void destroyLspDatas(LSP_Data* lsp_datas);

void LSP_destroyComputedData(LSP_ComputedData* lsp_payload);


//// ------------ UTILS ------------


void initLSPServerList(LSPServerLinkedList* list);

void destroyLSPServerList(LSPServerLinkedList* list);

LSP_Server* allocateNewLSPServerToLSPServerList(LSPServerLinkedList* list);

void addLSPServerCellToLSPServerList(LSPServerLinkedList* list, LSPServerLinkedList_Cell* cell);

LSP_Server* getLSPServerForLanguage(LSPServerLinkedList* list, char* language);

bool loadNewLSPServer(LSP_Server* container, char* language);


#endif // LSP_HANDLER_H
