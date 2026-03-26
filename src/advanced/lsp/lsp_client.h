/*
 * Partial implementation of :
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <linux/limits.h>
#include <stdbool.h>
#include <sys/types.h>

#include "../../../lib/cJSON/cJSON.h"

#define MESSAGE_LENGTH 4092
#define METHOD_MAX_LENGTH 200
#define LANGUAGE_ID_LENGTH 100

typedef enum { REQUEST, NOTIFICATION, RESPONSE } PACKET_TYPE;

typedef unsigned long LSP_PacketID;

struct _LSP_ResponseContext {
  LSP_PacketID id;
  char* method;
  char file_name[PATH_MAX];
  void* payload;
  struct _LSP_ResponseContext* next;
};

typedef struct _LSP_ResponseContext LSP_ResponseContext;

typedef struct {
  char name[LANGUAGE_ID_LENGTH];
  char language[LANGUAGE_ID_LENGTH];
  pid_t pid;
  int inpipefd[2];
  int outpipefd[2];
  cJSON* init_result;
  LSP_PacketID request_id;
  LSP_ResponseContext* response_contexts;
} LSP_Server;


//////// ---------------- JSON TOOLS ---------------------

void printToStdioJSON(cJSON* json);


//////// --------------- Low Layer JSON-RPC --------------------------

bool LSP_openLSPServer(char* name, char* command_args, char* language, LSP_Server* server);

void LSP_closeLSPServer(LSP_Server* server);

char* LSP_readPacket(LSP_Server* server);

cJSON* LSP_readPacketAsJSON(LSP_Server* server, bool block);

int LSP_sendPacket(LSP_Server* server, char* method, char* params, PACKET_TYPE type);

int LSP_sendPacketWithJSON(LSP_Server* server, char* method, cJSON* content, PACKET_TYPE type);

PACKET_TYPE LSP_getPacketType(cJSON* content);

void LSP_addResponseContext(LSP_Server* server, LSP_PacketID id, char* method, char* file_name, void* payload);
bool LSP_popResponseContext(LSP_Server* server, LSP_PacketID id, LSP_ResponseContext* context);
void LSP_clearResponseContext(LSP_Server* server);

//////// ----------------- Created Functions ---------------

//// -------- Tools Functions --------

void LSP_initializeServer(LSP_Server* lsp, char* client_name, char* client_version, char* current_workspace);

cJSON* LSP_extractPacketResult(cJSON* response_obj);

LSP_PacketID LSP_getPacketID(cJSON* request_body);

char* LSP_getPacketMethod(cJSON* request_body);

cJSON* LSP_getPacketResult(cJSON* request_body);

cJSON* LSP_getNotificationParams(cJSON* notification_body);

//// -------- JSON conversion --------*

typedef enum {
  LSP_GOTO_DECLARATION,
  LSP_GOTO_DEFINITION,
  LSP_GOTO_TYPE_DEFINITION,
  LSP_GOTO_IMPLEMENTATION,
  LSP_FIND_REFERENCE
} LSP_GOTO_TYPE;

typedef struct {
  int row;
  int column;
} Position;

Position LSP_getPositionOf(int cursor_row, int cursor_column);
cJSON* LSP_getJSONPosition(int cursor_row, int cursor_column);
Position LSP_getPositionFromJSON(cJSON* json);

typedef struct {
  Position pos1;
  Position pos2;
} Range;

Range LSP_getRangeOf(int cur1_row, int cur1_column, int cur2_row, int cur2_column);
cJSON* LSP_getJSONRange(int cur1_row, int cur1_column, int cur2_row, int cur2_column);
Range LSP_getRangeFromJSON(cJSON* json);

typedef struct {
  char file_name[PATH_MAX];
  char languageId[LANGUAGE_ID_LENGTH];
  int version;
  char* text;
} TextDocumentItem;

TextDocumentItem LSP_getTextDocumentItemOf(char* file_name, char* languageId, int version, char* text);
cJSON* LSP_getJSONTextDocumentItem(char* file_name, char* languageId, int version, char* text);
TextDocumentItem LSP_getTextDocumentItemFromJSON(cJSON* json);
void LSP_destroyTextDocumentItem(TextDocumentItem text_document_item);

typedef struct {
  char file_name[PATH_MAX];
} TextDocumentIdentifier;

TextDocumentIdentifier LSP_getTextDocumentIdentifierOf(char* file_name);
cJSON* LSP_getJSONTextDocumentIdentifier(char* file_name);
TextDocumentIdentifier LSP_getTextDocumentIdentifierFromJSON(cJSON* json);
void LSP_destroyTextDocumentIdentifier(TextDocumentIdentifier* text_document_identifier);

cJSON* LSP_getJSONTextDocumentIdentifierVersionned(char* file_name, int version);

typedef struct {
  TextDocumentIdentifier text_id;
  Position position;
} TextDocumentPositionParams;

TextDocumentPositionParams LSP_getTextDocumentPositionParamsOf(char* file_name, int cur_row, int cur_column);
cJSON* LSP_getJSONTextDocumentPositionParams(char* file_name, int cur_row, int cur_column);
TextDocumentPositionParams LSP_getTextDocumentPositionParamsFromJSON(cJSON* json);
void LSP_destroyTextDocumentPositionParams(TextDocumentPositionParams text_document_position_params);

typedef struct {
  Range range;
  char* new_text;
} TextEdit;

TextEdit LSP_getTextEditOf(int cur1_row, int cur1_column, int cur2_row, int cur2_column, char* new_text);
cJSON* LSP_getJSONTextEdit(int cur1_row, int cur1_column, int cur2_row, int cur2_column, char* new_text);
TextEdit LSP_getTextEditFromJSON(cJSON* json);
void LSP_destroyTextEdit(TextEdit text_edit);

typedef struct {
  TextDocumentIdentifier file_name;
  TextEdit edits[1];
} TextDocumentEdit;

TextDocumentEdit LSP_getTextDocumentEditOf(char* file_name, int cur1_row, int cur1_column, int cur2_row,
                                           int cur2_column, char* new_text);
cJSON* LSP_getJSONTextDocumentEdit(char* file_name, int cur1_row, int cur1_column, int cur2_row, int cur2_column,
                                   char* new_text);
TextDocumentEdit LSP_getTextDocumentEditFromJSON(cJSON* json);
void LSP_destroyTextDocumentEdit(TextDocumentEdit text_document_edit);

typedef struct {
  TextDocumentIdentifier file_name;
  Range range;
} Location;

Location LSP_getLocationOf(char* file_name, int cur1_row, int cur1_column, int cur2_row, int cur2_column);
cJSON* LSP_getJSONLocation(char* file_name, int cur1_row, int cur1_column, int cur2_row, int cur2_column);
Location LSP_getLocationFromJSON(cJSON* json);
void LSP_destroyLocation(Location* location);

typedef struct {
  Location* items;
  int size;
} LocationArray;

void LSP_getLocationArrayFromJSON(cJSON* json, LocationArray* array);
void LSP_destroyLocationArray(LocationArray* array);

typedef struct {
  Location location;
  char message[MESSAGE_LENGTH];
} DiagnosticRelatedInformation;

// TODO implement if needed
DiagnosticRelatedInformation LSP_getDiagnosticRelatedInformationOf(char* file_name, int cur1_row, int cur1_column,
                                                                   int cur2_row, int cur2_column);
cJSON* LSP_getJSONDiagnosticRelatedInformation(char* file_name, int cur1_row, int cur1_column, int cur2_row,
                                               int cur2_column);
DiagnosticRelatedInformation LSP_getDiagnosticRelatedInformationFromJSON(cJSON* json);
void LSP_destroyDiagnosticRelatedInformation(DiagnosticRelatedInformation location);

typedef enum { ERROR = 1, WARNING = 2, INFORMATION = 3, HINT = 4, SEVERITY_NONE = 0 } DiagnosticSeverity;
typedef enum { UNNECESSARY = 1, DEPRECATED = 2, TAG_NONE = 0 } DiagnosticTag;
typedef struct {
  DiagnosticSeverity severity;
  Range range;
  char source[100];
  char code[MESSAGE_LENGTH];
  char message[MESSAGE_LENGTH];
  char codeDescription[MESSAGE_LENGTH];
  // TODO implement if needed
  DiagnosticTag tags[100];
  // TODO implement if needed
  DiagnosticRelatedInformation infos[0];
} Diagnostic;

Diagnostic LSP_getDiagnosticOf(char* file_name, int cur1_row, int cur1_column, int cur2_row, int cur2_column);
cJSON* LSP_getJSONDiagnostic(char* file_name, int cur1_row, int cur1_column, int cur2_row, int cur2_column);
Diagnostic LSP_getDiagnosticFromJSON(cJSON* json);
void LSP_destroyDiagnostic(Diagnostic* diagnostic);


typedef enum {
  dt_PLAIN_TEXT,
  dt_MARKDOWN,
} DocumentationType;

typedef enum {
  ct_Text = 1,
  ct_Method = 2,
  ct_Function = 3,
  ct_Constructor = 4,
  ct_Field = 5,
  ct_Variable = 6,
  ct_Class = 7,
  ct_Interface = 8,
  ct_Module = 9,
  ct_Property = 10,
  ct_Unit = 11,
  ct_Value = 12,
  ct_Enum = 13,
  ct_Keyword = 14,
  ct_Snippet = 15,
  ct_Color = 16,
  ct_File = 17,
  ct_Reference = 18,
  ct_Folder = 19,
  ct_EnumMember = 20,
  ct_Constant = 21,
  ct_Struct = 22,
  ct_Event = 23,
  ct_Operator = 24,
  ct_TypeParameter = 25,
} CompletionType;

typedef enum { citf_PlainText = 1, citf_Snippet = 2 } CompletionInsertTextFormat;


struct _CompletionItem {
  char label[METHOD_MAX_LENGTH];
  char detail[METHOD_MAX_LENGTH];
  char description[MESSAGE_LENGTH];
  CompletionType kind;
  char documentation[MESSAGE_LENGTH];
  DocumentationType documentationType;
  char sortText[METHOD_MAX_LENGTH];
  char filterText[METHOD_MAX_LENGTH];
  char insertText[METHOD_MAX_LENGTH];
  TextEdit text_edit;
  bool is_text_edit;
  TextEdit* additionalTextEdits;
  int additionalTextEditsSize;
};

typedef struct _CompletionItem CompletionItem;

typedef struct {
  int size;
  CompletionItem* items;
} CompletionArray;

typedef struct {
  bool isIncomplete;
  CompletionArray completions;
} CompletionList;

void LSP_getCompletionListFromJSON(cJSON* json, CompletionList* list);
void LSP_getCompletionArrayFromJSON(cJSON* json, CompletionArray* array);
void LSP_getCompletionItemFromJSON(cJSON* json, CompletionItem* item);
void LSP_destroyCompletionItem(CompletionItem* item);
void LSP_destroyCompletionList(CompletionList* completion_list);


typedef struct MarkedString {
  char value[MESSAGE_LENGTH];
  DocumentationType documentationType;
} MarkedString;

typedef struct Hover {
  MarkedString* contents;
  int size;
  bool is_range;
  Range range;
} Hover;


void LSP_getHoverFromJSON(cJSON* json, Hover* hover_list);
void LSP_getMarkedStringFromJSON(cJSON* json, MarkedString* item);
void LSP_destroyHover(Hover* hover_list);


//// -------- Receive Functions --------

bool LSP_dispatchOnReceive(LSP_Server* lsp, void (*dispatcher)(cJSON* packet, LSP_Server* lsp, void* payload),
                           void* payload);


//// -------- Notify Functions --------


void LSP_notifyLspFileDidOpen(LSP_Server* lsp, char* file_name, char* file_content);
void LSP_notifyLspFileDidChange(LSP_Server* lsp, char* file_name, cJSON* array_of_changes, int version);


//// -------- Request Functions --------


void LSP_requestCompletion(LSP_Server* lsp, char* file_name, int row, int column);
void LSP_requestHover(LSP_Server* lsp, char* file_name, int row, int column);
void LSP_requestGoto(LSP_Server* lsp, char* file_name, int row, int column, LSP_GOTO_TYPE goto_type);

#endif // CLIENT_H
