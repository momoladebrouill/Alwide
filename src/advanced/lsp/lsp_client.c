#include "lsp_client.h"

#include <assert.h>
#include <libgen.h>
#include <linux/limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../../utils/tools.h"


//////// ---------------- JSON TOOLS ---------------------

void printToStdioJSON(cJSON* json) {
  char* text = cJSON_Print(json);
  printf("%s\n", text);
  free(text);
}


//////// --------------- Low Layer JSON-RPC --------------------------


// https://stackoverflow.com/questions/6171552/popen-simultaneous-read-and-write
bool LSP_openLSPServer(char* name, char* command_args, char* language, LSP_Server* server) {
  char* path = whereis(name);
  if (path == NULL) {
    fprintf(stderr, "LSP server : '%s' wasn't found ! LSP abort for language %s\n", name, language);
    return false;
  }
  char pathMemSafe[PATH_MAX];

  strcpy(server->name, name);
  strcpy(pathMemSafe, path);

  free(path);

  strlcpy(server->language, language, 100);

  server->request_id = 0;
  server->response_contexts = NULL;
  server->pending_packets = NULL;
  pthread_mutex_init(&server->init_done, NULL);
  pthread_mutex_lock(&server->init_done);
  pthread_mutex_init(&server->pending_lock, NULL);

  // printf("Starting server on path : %s\n", pathMemSafe);

  // Create pipe in the 2 directions.
  pipe(server->inpipefd);
  pipe(server->outpipefd);

  server->pid = fork();
  if (server->pid == 0) {
    // Child

    // Bind Stdin/Stdout on pipes.
    dup2(server->outpipefd[0], STDIN_FILENO);
    dup2(server->inpipefd[1], STDOUT_FILENO);
    // dup2(server->inpipefd[1], STDERR_FILENO); // Do not bind the err channel it's used for lsp_server logs.


    // ask kernel to deliver SIGTERM in case the parent dies
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    // close unused pipe ends
    close(server->outpipefd[1]);
    close(server->inpipefd[0]);


    char command[strlen(name) + strlen(command_args) + strlen(" 2>> .lsp_logs.txt ") + 1 /*null char*/];
    sprintf(command, "%s %s 2>> .lsp_logs.txt", name, command_args);

    // system(command);
    // execl(pathMemSafe, "", (char *)NULL);

    execl("/bin/sh", "sh", "-c", command, NULL);

    exit(1);
  }
  // The code below will be executed only by parent. You can write and read
  // from the child using pipefd descriptors, and you can send signals to
  // the process using its pid by kill() function. If the child process will
  // exit unexpectedly, the parent process will obtain SIGCHLD signal that
  // can be handled (e.g. you can respawn the child process).

  // close unused pipe ends
  close(server->outpipefd[0]);
  close(server->inpipefd[1]);

  return true;
}


void LSP_closeLSPServer(LSP_Server* server) {
  kill(server->pid, SIGKILL); // send SIGKILL signal to the child process
  int status;
  waitpid(server->pid, &status, 0);
  cJSON_Delete(server->init_result);
  LSP_clearResponseContext(server);

  pthread_mutex_lock(&server->pending_lock);
  LSP_PendingPacket* curr = server->pending_packets;
  while (curr != NULL) {
    LSP_PendingPacket* tmp = curr;
    curr = curr->next;
    free(tmp->method);
    free(tmp->params);
    free(tmp);
  }
  server->pending_packets = NULL;
  pthread_mutex_unlock(&server->pending_lock);

  pthread_mutex_destroy(&server->init_done);
  pthread_mutex_destroy(&server->pending_lock);
}


void skipUntilContent(LSP_Server* server) {}

#define BUFF_SIZE 256
#define HEADER_FIRST_FIELD "Content-Length:"

char* LSP_readPacket(LSP_Server* server) {
  char buf[BUFF_SIZE];

  // read the first head field title.
  int header_size = strlen(HEADER_FIRST_FIELD);
  int n = read(server->inpipefd[0], buf, header_size);

  if (n <= 0) {
    return NULL;
  }
  buf[n] = '\0';

  // If the first header is not the expected one.
  if (n != header_size || strcmp(HEADER_FIRST_FIELD, buf) != 0) {
    fprintf(stderr, "Error with lsp read : expected '%s' but got '%s'.\r\n", HEADER_FIRST_FIELD, buf);
    return NULL;
  }

  // save the value of the first field in buf.
  int index = 0;
  while (index < BUFF_SIZE - 1) {
    if (poll(&(struct pollfd){.fd = server->inpipefd[0], .events = POLLIN}, 1, 100) != 1) {
      break;
    }
    n = read(server->inpipefd[0], buf + index, 1);
    if (n <= 0 || buf[index] == '\n') {
      break;
    }
    index++;
  }
  buf[index] = '\0';

  // Extract the content_length from buf.
  int content_length = 0;
  if (sscanf(buf, " %d ", &content_length) != 1) {
    fprintf(stderr, "Error with lsp read : couldn't extract content length from '%s'.\r\n", buf);
    return NULL;
  }

  // Extract the full second field. (This field is ignored).
  index = 0;
  while (index < BUFF_SIZE - 1) {
    if (poll(&(struct pollfd){.fd = server->inpipefd[0], .events = POLLIN}, 1, 100) != 1) {
      break;
    }
    n = read(server->inpipefd[0], buf + index, 1);
    if (n <= 0 || buf[index] == '\n') {
      break;
    }
    index++;
  }

  // reach first '{' some lsp servers add more break lines...  :/
  index = 0;
  while (index < BUFF_SIZE - 1) {
    if (poll(&(struct pollfd){.fd = server->inpipefd[0], .events = POLLIN}, 1, 100) != 1) {
      break;
    }
    n = read(server->inpipefd[0], buf, 1);
    if (n <= 0 || buf[0] == '{') {
      break;
    }
  }

  // Allocate the content array
  char* content = malloc(content_length * sizeof(char) + 1 /* +1 for null char*/);
  if (content == NULL) {
    return NULL;
  }

  content[0] = '{'; // We already read the first '{'
  n = 0;
  while (n < content_length - 1) {
    if (poll(&(struct pollfd){.fd = server->inpipefd[0], .events = POLLIN}, 1, 1000) != 1) {
      fprintf(stderr, "LSP timeout while reading body.\n");
      break;
    }
    int r = read(server->inpipefd[0], content + 1 + n, content_length - 1 - n);
    if (r <= 0) {
      break;
    }
    n += r;
  }
  content[n + 1] = '\0';

  return content;
}

cJSON* LSP_readPacketAsJSON(LSP_Server* server, bool block) {
  if (block == false && poll(&(struct pollfd){.fd = server->inpipefd[0], .events = POLLIN}, 1, 0) != 1) {
    return NULL;
  }
  char* content_str = LSP_readPacket(server);
  cJSON* at_return = cJSON_Parse(content_str);
  if (LSP_getPacketType(at_return) != LSP_RESPONSE) {
    if (strcmp("window/logMessage", LSP_getPacketMethod(at_return)) == 0) {
      cJSON* message_obj = cJSON_GetObjectItem(LSP_getNotificationParams(at_return), "message");
      printf("Server log : %s\n", cJSON_GetStringValue(message_obj));
      cJSON_Delete(at_return);
      free(content_str);
      return LSP_readPacketAsJSON(server, block);
    }
  }
  free(content_str);
  return at_return;
}

static int _LSP_sendPacketInternal(LSP_Server* server, char* method, char* params, LSP_PACKET_TYPE type, LSP_PacketID id) {
  if (params == NULL) {
    params = "{}";
  }
  cJSON* json_request_obj = cJSON_CreateObject();
  cJSON_AddStringToObject(json_request_obj, "jsonrpc", "2.0");
  cJSON_AddStringToObject(json_request_obj, "method", method);
  cJSON_AddRawToObject(json_request_obj, "params", params);
  if (type == LSP_REQUEST) {
    cJSON_AddNumberToObject(json_request_obj, "id", id);
  }

  char* content_str = cJSON_PrintUnformatted(json_request_obj);
  int content_length = strlen(content_str);

  char atSend[content_length + 200];

  snprintf(atSend, sizeof(atSend), "Content-Length: %d\r\n\r\n%s", content_length, content_str);
  int head_length = strlen(atSend) - content_length;

  write(server->outpipefd[1], atSend, head_length + content_length);

  // fwrite(atSend, 1, head_length + content_length, stdout);
  // printf("\n");

  // TODO remove
  fprintf(stderr, "\n\n ================ %s ================>>> \n",
          cJSON_GetStringValue(cJSON_GetObjectItem(json_request_obj, "method")));
  cJSON* params_2 = cJSON_Parse(params);
  char* text = cJSON_Print(params_2);
  fprintf(stderr, "%s\n", text);
  free(text);
  cJSON_Delete(params_2);
  // TODO end remove

  free(content_str);
  cJSON_Delete(json_request_obj);

  if (type == LSP_REQUEST) {
    return id;
  }
  return 0;
}

int LSP_sendPacket(LSP_Server* server, char* method, char* params, LSP_PACKET_TYPE type) {
  if (strcmp(method, "initialize") != 0) {
    if (pthread_mutex_trylock(&server->init_done) != 0) {
      // Buffer packet
      pthread_mutex_lock(&server->pending_lock);
      LSP_PendingPacket* new_packet = malloc(sizeof(LSP_PendingPacket));
      new_packet->method = strdup(method);
      new_packet->params = strdup(params ? params : "{}");
      new_packet->type = type;
      new_packet->id = 0;
      if (type == LSP_REQUEST) {
        server->request_id++;
        new_packet->id = server->request_id;
      }
      new_packet->next = NULL;

      if (server->pending_packets == NULL) {
        server->pending_packets = new_packet;
      }
      else {
        LSP_PendingPacket* curr = server->pending_packets;
        while (curr->next != NULL) {
          curr = curr->next;
        }
        curr->next = new_packet;
      }
      int returned_id = new_packet->id;
      pthread_mutex_unlock(&server->pending_lock);
      return returned_id;
    }
    else {
      pthread_mutex_unlock(&server->init_done);
    }
  }

  LSP_PacketID packet_id = 0;
  if (type == LSP_REQUEST) {
    server->request_id++;
    packet_id = server->request_id;
  }

  return _LSP_sendPacketInternal(server, method, params, type, packet_id);
}

int LSP_sendPacketWithJSON(LSP_Server* server, char* method, cJSON* content, LSP_PACKET_TYPE type) {
  char* json_params = cJSON_PrintUnformatted(content);
  int temp_id = LSP_sendPacket(server, method, json_params, type);
  free(json_params);
  return temp_id;
}

LSP_PACKET_TYPE LSP_getPacketType(cJSON* content) {
  cJSON* id_obj = cJSON_GetObjectItem(content, "id");
  cJSON* method_obj = cJSON_GetObjectItem(content, "method");
  if (id_obj != NULL && method_obj == NULL) {
    return LSP_RESPONSE;
  }
  if (method_obj != NULL && id_obj == NULL) {
    return LSP_NOTIFICATION;
  }
  if (method_obj != NULL && id_obj != NULL) {
    return LSP_REQUEST;
  }

  // If there something wrong with server happened. Or something not handled.
  assert(false);
}


void LSP_addResponseContext(LSP_Server* server, LSP_PacketID id, char* method, char* file_name, void* payload) {
  LSP_ResponseContext* new_context = malloc(sizeof(LSP_ResponseContext));
  new_context->next = server->response_contexts;
  new_context->id = id;
  new_context->method = method;
  strlcpy(new_context->file_name, file_name, PATH_MAX);
  new_context->payload = payload;

  server->response_contexts = new_context;
}


bool LSP_popResponseContext(LSP_Server* server, LSP_PacketID id, LSP_ResponseContext* context) {
  if (server->response_contexts->id == id) {
    *context = *server->response_contexts;
    LSP_ResponseContext* tmp = server->response_contexts;
    server->response_contexts = server->response_contexts->next;
    free(tmp);
    return true;
  }

  LSP_ResponseContext* current = server->response_contexts;
  while (current->next != NULL) {
    if (current->next->id == id) {
      *context = *current->next;
      LSP_ResponseContext* tmp = current->next;
      current->next = tmp->next;
      free(tmp);
      return true;
    }
    current = current->next;
  }

  return false;
}

void LSP_clearResponseContext(LSP_Server* server) {
  LSP_ResponseContext* current = server->response_contexts;
  while (current != NULL) {
    LSP_ResponseContext* tmp = current;
    current = current->next;
    free(tmp->payload);
    free(tmp);
  }
}


//////// ----------------- Created Functions ---------------


//// -------- Tools Functions --------

// TODO Implement send of Capabality
// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#initialize
void LSP_initializeServer(LSP_Server* lsp, char* client_name, char* client_version, char* current_workspace_path) {
  cJSON* init_params = cJSON_CreateObject();
  cJSON_AddNumberToObject(init_params, "processId", lsp->pid);

  char workspaceURI[URI_MAX];
  getLocalURI(current_workspace_path, workspaceURI);

  cJSON_AddStringToObject(init_params, "rootUri", workspaceURI);
  cJSON_AddStringToObject(init_params, "rootPath", current_workspace_path);

  cJSON* client_info = cJSON_AddObjectToObject(init_params, "clientInfo");
  cJSON_AddStringToObject(client_info, "name", client_name);
  cJSON_AddStringToObject(client_info, "version", client_version);

  cJSON* workspace_array = cJSON_AddArrayToObject(init_params, "workspaceFolders");

  cJSON* workspace = cJSON_CreateObject();
  cJSON_AddStringToObject(workspace, "uri", workspaceURI);
  cJSON_AddStringToObject(workspace, "name", basename(current_workspace_path));
  cJSON_AddItemToArray(workspace_array, workspace);

  cJSON* general = cJSON_AddObjectToObject(init_params, "general");
  cJSON* positionEncodings = cJSON_AddArrayToObject(general, "positionEncodings");
  cJSON_AddItemToArray(positionEncodings, cJSON_CreateString("utf-8"));

  int tmp_id = LSP_sendPacketWithJSON(lsp, "initialize", init_params, LSP_REQUEST);

  char* init_params_text = cJSON_Print(init_params);
  fprintf(stderr, "INIT_SEND:\n%s\n", init_params_text);
  free(init_params_text);

  cJSON_Delete(init_params);

  cJSON* content = LSP_readPacketAsJSON(lsp, true);
  // this assert will also check if the packet is a request.
  assert(tmp_id == LSP_getPacketID(content));
  lsp->init_result = LSP_extractPacketResult(content);
  char* init_text = cJSON_Print(lsp->init_result);
  fprintf(stderr, "INIT: \n%s\n", init_text);
  free(init_text);
  cJSON_Delete(content);
  pthread_mutex_unlock(&lsp->init_done);

  // Flush pending notifications
  pthread_mutex_lock(&lsp->pending_lock);
  LSP_PendingPacket* curr = lsp->pending_packets;
  while (curr != NULL) {
    _LSP_sendPacketInternal(lsp, curr->method, curr->params, curr->type, curr->id);
    LSP_PendingPacket* tmp = curr;
    curr = curr->next;
    free(tmp->method);
    free(tmp->params);
    free(tmp);
  }
  lsp->pending_packets = NULL;
  pthread_mutex_unlock(&lsp->pending_lock);
}


cJSON* LSP_extractPacketResult(cJSON* response_obj) {
  cJSON* at_return = cJSON_GetObjectItem(response_obj, "result");
  cJSON_DetachItemFromObject(response_obj, "result");
  return at_return;
}

LSP_PacketID LSP_getPacketID(cJSON* request_body) {
  assert(LSP_getPacketType(request_body) == LSP_REQUEST || LSP_getPacketType(request_body) == LSP_RESPONSE);
  cJSON* id_obj = cJSON_GetObjectItem(request_body, "id");
  assert(id_obj != NULL);
  unsigned long current_id = (unsigned long)cJSON_GetNumberValue(id_obj);
  return current_id;
}

char* LSP_getPacketMethod(cJSON* request_body) {
  assert(LSP_getPacketType(request_body) == LSP_REQUEST || LSP_getPacketType(request_body) == LSP_NOTIFICATION);
  cJSON* method_obj = cJSON_GetObjectItem(request_body, "method");
  assert(method_obj != NULL);
  char* method_name = cJSON_GetStringValue(method_obj);
  return method_name;
}

cJSON* LSP_getPacketResult(cJSON* request_body) {
  assert(LSP_getPacketType(request_body) == LSP_REQUEST || LSP_getPacketType(request_body) == LSP_RESPONSE);
  cJSON* result = cJSON_GetObjectItem(request_body, "result");
  assert(result != NULL);
  return result;
}


cJSON* LSP_getNotificationParams(cJSON* notification_body) {
  assert(LSP_getPacketType(notification_body) == LSP_NOTIFICATION);
  cJSON* param_obj = cJSON_GetObjectItem(notification_body, "params");
  return param_obj;
}

LSP_Position LSP_pos(int lsp_row, int lsp_col) { return (LSP_Position){.row = lsp_row, .column = lsp_col}; }

LSP_Position LSP_getPositionFromJSON(cJSON* json) {
  return LSP_pos(cJSON_GetNumberValue(cJSON_GetObjectItem(json, "line")),
                 cJSON_GetNumberValue(cJSON_GetObjectItem(json, "character")));
}

cJSON* LSP_getJSONPosition(LSP_Position pos) {
  cJSON* position = cJSON_CreateObject();
  cJSON_AddNumberToObject(position, "line", pos.row);
  cJSON_AddNumberToObject(position, "character", pos.column);

  return position;
}


LSP_Range LSP_range(LSP_Position p1, LSP_Position p2) { return (LSP_Range){.pos1 = p1, .pos2 = p2}; }

LSP_Range LSP_getRangeFromJSON(cJSON* json) {
  return LSP_range(LSP_getPositionFromJSON(cJSON_GetObjectItem(json, "start")),
                   LSP_getPositionFromJSON(cJSON_GetObjectItem(json, "end")));
}

cJSON* LSP_getJSONRange(LSP_Range range) {
  cJSON* range_json = cJSON_CreateObject();
  cJSON_AddItemToObject(range_json, "start", LSP_getJSONPosition(range.pos1));
  cJSON_AddItemToObject(range_json, "end", LSP_getJSONPosition(range.pos2));

  return range_json;
}

LSP_TextDocumentItem LSP_getTextDocumentItemOf(char* file_name, char* languageId, int version, char* text) {
  LSP_TextDocumentItem text_document_item;
  strncpy(text_document_item.file_name, file_name, PATH_MAX);
  strncpy(text_document_item.languageId, languageId, LANGUAGE_ID_LENGTH);
  text_document_item.version = version;
  text_document_item.text = strdup(text);

  return text_document_item;
}

cJSON* LSP_getJSONTextDocumentItem(char* file_name, char* languageId, int version, char* text) {
  cJSON* text_document = cJSON_CreateObject();

  char uri[URI_MAX];
  getLocalURI(file_name, uri);
  cJSON_AddStringToObject(text_document, "uri", uri);
  cJSON_AddStringToObject(text_document, "languageId", languageId);
  cJSON_AddNumberToObject(text_document, "version", version);

  cJSON_AddStringToObject(text_document, "text", text);

  return text_document;
}

LSP_TextDocumentItem LSP_getTextDocumentItemFromJSON(cJSON* json) {
  return LSP_getTextDocumentItemOf(cJSON_GetStringValue(cJSON_GetObjectItem(json, "uri")),
                                   cJSON_GetStringValue(cJSON_GetObjectItem(json, "languageId")),
                                   cJSON_GetNumberValue(cJSON_GetObjectItem(json, "version")),
                                   cJSON_GetStringValue(cJSON_GetObjectItem(json, "text")));
}

void LSP_destroyTextDocumentItem(LSP_TextDocumentItem text_document_item) { free(text_document_item.text); }

LSP_TextDocumentIdentifier LSP_getTextDocumentIdentifierOf(char* file_name) {
  LSP_TextDocumentIdentifier text_id;
  strncpy(text_id.file_name, file_name, PATH_MAX);

  return text_id;
}

cJSON* LSP_getJSONTextDocumentIdentifier(char* file_name) {
  cJSON* text_document_id = cJSON_CreateObject();
  char uri[URI_MAX];
  getLocalURI(file_name, uri);
  cJSON_AddStringToObject(text_document_id, "uri", uri);

  return text_document_id;
}

cJSON* LSP_getJSONTextDocumentIdentifierVersionned(char* file_name, int version) {
  cJSON* text_document_id = cJSON_CreateObject();
  char uri[URI_MAX];
  getLocalURI(file_name, uri);
  cJSON_AddStringToObject(text_document_id, "uri", uri);
  cJSON_AddNumberToObject(text_document_id, "version", version);

  return text_document_id;
}

LSP_TextDocumentIdentifier LSP_getTextDocumentIdentifierFromJSON(cJSON* json) {
  return LSP_getTextDocumentIdentifierOf(cJSON_GetStringValue(cJSON_GetObjectItem(json, "uri")));
}

void LSP_destroyTextDocumentIdentifier(LSP_TextDocumentIdentifier* text_document_identifier) {}

LSP_TextDocumentPositionParams LSP_getTextDocumentPositionParamsOf(char* file_name, LSP_Position pos) {
  LSP_TextDocumentPositionParams text_document_position_params;
  text_document_position_params.text_id = LSP_getTextDocumentIdentifierOf(file_name);
  text_document_position_params.position = pos;

  return text_document_position_params;
}

cJSON* LSP_getJSONTextDocumentPositionParams(char* file_name, LSP_Position pos) {
  cJSON* text_document_position_params = cJSON_CreateObject();

  cJSON* text_document_id = LSP_getJSONTextDocumentIdentifier(file_name);
  cJSON_AddItemToObject(text_document_position_params, "textDocument", text_document_id);

  cJSON* position = LSP_getJSONPosition(pos);
  cJSON_AddItemToObject(text_document_position_params, "position", position);

  return text_document_position_params;
}

LSP_TextDocumentPositionParams LSP_getTextDocumentPositionParamsFromJSON(cJSON* json) {
  LSP_TextDocumentIdentifier text_id = LSP_getTextDocumentIdentifierFromJSON(cJSON_GetObjectItem(json, "textDocument"));
  LSP_Position position = LSP_getPositionFromJSON(cJSON_GetObjectItem(json, "position"));
  return LSP_getTextDocumentPositionParamsOf(text_id.file_name, position);
}

void LSP_destroyTextDocumentPositionParams(LSP_TextDocumentPositionParams text_document_position_params) {
  LSP_destroyTextDocumentIdentifier(&text_document_position_params.text_id);
}


LSP_TextEdit LSP_getTextEditOf(LSP_Range range, char* new_text) {
  LSP_TextEdit text_edit;
  text_edit.range = range;
  text_edit.new_text = strdup(new_text);
  return text_edit;
}


cJSON* LSP_getJSONTextEdit(LSP_Range range, char* new_text) {
  cJSON* text_edit = cJSON_CreateObject();

  cJSON* range_json = LSP_getJSONRange(range);
  cJSON_AddItemToObject(text_edit, "range", range_json);

  cJSON_AddStringToObject(text_edit, "newText", new_text);

  return text_edit;
}

LSP_TextEdit LSP_getTextEditFromJSON(cJSON* json) {
  LSP_Range range = LSP_getRangeFromJSON(cJSON_GetObjectItem(json, "range"));
  return LSP_getTextEditOf(range, cJSON_GetStringValue(cJSON_GetObjectItem(json, "newText")));
}

void LSP_destroyTextEdit(LSP_TextEdit text_edit) { free(text_edit.new_text); }

LSP_TextDocumentEdit LSP_getTextDocumentEditOf(char* file_name, LSP_Range range, char* new_text) {
  LSP_TextDocumentEdit text_document_edit;
  text_document_edit.file_name = LSP_getTextDocumentIdentifierOf(file_name);
  text_document_edit.edits[0] = LSP_getTextEditOf(range, new_text);
  return text_document_edit;
}


cJSON* LSP_getJSONTextDocumentEdit(char* file_name, LSP_Range range, char* new_text) {
  cJSON* text_document_edit = cJSON_CreateObject();

  cJSON* text_document_id = LSP_getJSONTextDocumentIdentifier(file_name);
  cJSON_AddItemToObject(text_document_edit, "textDocument", text_document_id);

  cJSON* edits = cJSON_AddArrayToObject(text_document_edit, "edits");
  cJSON* edit = LSP_getJSONTextEdit(range, new_text);
  cJSON_AddItemToArray(edits, edit);

  return text_document_edit;
}

LSP_TextDocumentEdit LSP_getTextDocumentEditFromJSON(cJSON* json) {
  LSP_TextEdit text_edit = LSP_getTextEditFromJSON(cJSON_GetArrayItem(cJSON_GetObjectItem(json, "edits"), 0));
  LSP_TextDocumentIdentifier text_id = LSP_getTextDocumentIdentifierFromJSON(cJSON_GetObjectItem(json, "textDocument"));
  return LSP_getTextDocumentEditOf(text_id.file_name, text_edit.range, text_edit.new_text);
}

LSP_Location LSP_getLocationOf(char* file_name, LSP_Range range) {
  LSP_Location location;
  location.file_name = LSP_getTextDocumentIdentifierOf(file_name);
  location.range = range;
  return location;
}

cJSON* LSP_getJSONLocation(char* file_name, LSP_Range range) {
  cJSON* location = cJSON_CreateObject();

  char uri[URI_MAX];
  getLocalURI(file_name, uri);
  cJSON_AddStringToObject(location, "uri", uri);

  cJSON* range_json = LSP_getJSONRange(range);
  cJSON_AddItemToObject(location, "range", range_json);

  return location;
}

LSP_Location LSP_getLocationFromJSON(cJSON* json) {
  LSP_TextDocumentIdentifier text_id = LSP_getTextDocumentIdentifierFromJSON(cJSON_GetObjectItem(json, "uri"));
  LSP_Range range = LSP_getRangeFromJSON(cJSON_GetObjectItem(json, "range"));

  return LSP_getLocationOf(text_id.file_name, range);
}

void LSP_destroyLocation(LSP_Location* location) { LSP_destroyTextDocumentIdentifier(&location->file_name); }


// Internal helper for handling Location and LocationLink
LSP_Location LSP_getLocationOrLocationLinkFromJSON_internal(cJSON* json) {
  cJSON* uri_obj = cJSON_GetObjectItem(json, "uri");
  if (uri_obj == NULL) {
    uri_obj = cJSON_GetObjectItem(json, "targetUri");
  }

  // we abstract the range system to just save the range where to jump. Prioritizing precises ranges.
  cJSON* range_obj = cJSON_GetObjectItem(json, "range");
  if (range_obj == NULL) {
    range_obj = cJSON_GetObjectItem(json, "targetSelectionRange");
  }
  if (range_obj == NULL) {
    range_obj = cJSON_GetObjectItem(json, "targetRange");
  }

  char* uri = cJSON_GetStringValue(uri_obj);
  char decoded_path[PATH_MAX];
  decodeURI(uri, decoded_path, PATH_MAX);

  LSP_TextDocumentIdentifier text_id;
  strncpy(text_id.file_name, decoded_path, PATH_MAX);
  LSP_Range range = LSP_getRangeFromJSON(range_obj);

  return (LSP_Location){.file_name = text_id, .range = range};
}

void LSP_getLocationArrayFromJSON(cJSON* json, LSP_LocationArray* array) {
  if (json == NULL || cJSON_IsNull(json)) {
    array->size = 0;
    array->items = NULL;
    return;
  }

  if (cJSON_IsArray(json)) {
    int count = cJSON_GetArraySize(json);
    array->size = count;
    array->items = malloc(sizeof(LSP_Location) * count);
    for (int i = 0; i < count; i++) {
      cJSON* item = cJSON_GetArrayItem(json, i);
      // Handle Location or LocationLink
      cJSON* uri = cJSON_GetObjectItem(item, "uri");
      if (uri == NULL) {
        // LocationLink has targetUri instead
        uri = cJSON_GetObjectItem(item, "targetUri");
      }
      cJSON* range = cJSON_GetObjectItem(item, "range");
      if (range == NULL) {
        // LocationLink has targetRange or targetSelectionRange
        range = cJSON_GetObjectItem(item, "targetRange");
      }

      array->items[i] = LSP_getLocationOrLocationLinkFromJSON_internal(item);
    }
  }
  else {
    array->size = 1;
    array->items = malloc(sizeof(LSP_Location));
    array->items[0] = LSP_getLocationOrLocationLinkFromJSON_internal(json);
  }
}


void LSP_destroyLocationArray(LSP_LocationArray* array) {
  if (array->items) {
    for (int i = 0; i < array->size; i++) {
      LSP_destroyLocation(array->items + i);
    }
    free(array->items);
    array->items = NULL;
  }
  array->size = 0;
}


void initDiagnostic(LSP_Diagnostic* diagnostic) {
  diagnostic->code[0] = '\0';
  diagnostic->message[0] = '\0';
  diagnostic->codeDescription[0] = '\0';
  diagnostic->tags[0] = LSP_DIAG_TAG_NONE;
  diagnostic->severity = LSP_DIAG_SEVERITY_NONE;
  diagnostic->source[0] = '\0';
}

LSP_Diagnostic LSP_getDiagnosticOf(LSP_Range range) {
  LSP_Diagnostic diagnostic;
  initDiagnostic(&diagnostic);
  diagnostic.range = range;
  return diagnostic;
}

cJSON* LSP_getJSONDiagnostic(LSP_Range range) {
  cJSON* diagnostic = cJSON_CreateObject();
  cJSON_AddItemToObject(diagnostic, "range", LSP_getJSONRange(range));
  return diagnostic;
}

LSP_Diagnostic LSP_getDiagnosticFromJSON(cJSON* json) {
  LSP_Diagnostic diagnostic;
  initDiagnostic(&diagnostic);

  diagnostic.range = LSP_getRangeFromJSON(cJSON_GetObjectItem(json, "range"));

  cJSON* code = cJSON_GetObjectItem(json, "code");
  if (code) {
    if (cJSON_GetStringValue(code)) {
      strlcpy(diagnostic.code, cJSON_GetStringValue(code), MESSAGE_LENGTH);
    }
  }

  cJSON* severity = cJSON_GetObjectItem(json, "severity");
  if (severity) {
    diagnostic.severity = (int)cJSON_GetNumberValue(severity);
  }

  cJSON* message = cJSON_GetObjectItem(json, "message");
  if (message) {
    if (cJSON_GetStringValue(message)) {
      strlcpy(diagnostic.message, cJSON_GetStringValue(message), MESSAGE_LENGTH);
    }
  }

  cJSON* codeDescription = cJSON_GetObjectItem(json, "codeDescription");
  if (codeDescription) {
    if (cJSON_GetStringValue(codeDescription)) {
      strlcpy(diagnostic.codeDescription, cJSON_GetStringValue(codeDescription), MESSAGE_LENGTH);
    }
  }

  return diagnostic;
}

void LSP_destroyDiagnostic(LSP_Diagnostic* diagnostic) {}


void LSP_getCompletionListFromJSON(cJSON* json, LSP_CompletionList* list) {
  assert(json != NULL);

  // isIncomplete
  cJSON* isIncompleteItem = cJSON_GetObjectItem(json, "isIncomplete");
  list->isIncomplete = !(isIncompleteItem == NULL || cJSON_IsFalse(isIncompleteItem));

  LSP_getCompletionArrayFromJSON(cJSON_GetObjectItem(json, "items"), &list->completions);
}


void LSP_getCompletionArrayFromJSON(cJSON* json, LSP_CompletionArray* array) {
  array->items = NULL;
  array->size = 0;

  if (json == NULL) {
    return;
  }

  array->size = cJSON_GetArraySize(json);
  array->items = malloc(array->size * sizeof(LSP_CompletionItem));
  for (int i = 0; i < array->size; i++) {
    LSP_getCompletionItemFromJSON(cJSON_GetArrayItem(json, i), array->items + i);
  }
}


void LSP_getCompletionItemFromJSON(cJSON* json, LSP_CompletionItem* item) {
  assert(item != NULL);

  // defaults
  item->detail[0] = '\0';
  item->description[0] = '\0';
  item->kind = ct_Text;
  item->documentation[0] = '\0';
  item->documentationType = dt_PLAIN_TEXT;
  item->is_text_edit = false;
  item->additionalTextEdits = NULL;
  item->additionalTextEditsSize = 0;

  // copy the label
  assert(cJSON_GetObjectItem(json, "label") != NULL);
  strlcpy(item->label, cJSON_GetStringValue(cJSON_GetObjectItem(json, "label")), METHOD_MAX_LENGTH);

  // fill if present labelDetails
  cJSON* tmp_item;
  if ((tmp_item = cJSON_GetObjectItem(json, "labelDetails")) != NULL) {
    cJSON* tmp_item_2;
    if ((tmp_item_2 = cJSON_GetObjectItem(tmp_item, "detail")) != NULL) {
      strlcpy(item->detail, cJSON_GetStringValue(tmp_item_2), METHOD_MAX_LENGTH);
    }
    if ((tmp_item_2 = cJSON_GetObjectItem(tmp_item, "description")) != NULL) {
      strlcpy(item->description, cJSON_GetStringValue(tmp_item_2), METHOD_MAX_LENGTH);
    }
  }

  // fill if present detail
  if ((tmp_item = cJSON_GetObjectItem(tmp_item, "detail")) != NULL) {
    strlcpy(item->detail, cJSON_GetStringValue(tmp_item), METHOD_MAX_LENGTH);
  }

  // fill if present kind
  if ((tmp_item = cJSON_GetObjectItem(json, "kind")) != NULL) {
    item->kind = cJSON_GetNumberValue(tmp_item);
  }

  // fill if present the documentation
  if ((tmp_item = cJSON_GetObjectItem(json, "documentation"))) {
    if (cJSON_IsString(tmp_item)) {
      strlcpy(item->documentation, cJSON_GetStringValue(tmp_item), MESSAGE_LENGTH);
    }
    else {
      item->documentationType = cJSON_GetNumberValue(cJSON_GetObjectItem(tmp_item, "kind"));
      strlcpy(item->documentation, cJSON_GetStringValue(cJSON_GetObjectItem(tmp_item, "value")), MESSAGE_LENGTH);
    }
  }

  // fill if present sortText
  if ((tmp_item = cJSON_GetObjectItem(json, "sortText"))) {
    strlcpy(item->sortText, cJSON_GetStringValue(tmp_item), METHOD_MAX_LENGTH);
  }
  else {
    strncpy(item->sortText, item->label, METHOD_MAX_LENGTH);
  }

  // fill if present filterText
  if ((tmp_item = cJSON_GetObjectItem(json, "filterText"))) {
    strlcpy(item->filterText, cJSON_GetStringValue(tmp_item), METHOD_MAX_LENGTH);
  }
  else {
    strncpy(item->filterText, item->label, METHOD_MAX_LENGTH);
  }

  // fill if present insertText
  if ((tmp_item = cJSON_GetObjectItem(json, "insertText"))) {
    strlcpy(item->insertText, cJSON_GetStringValue(tmp_item), METHOD_MAX_LENGTH);
  }
  else {
    strncpy(item->insertText, item->label, METHOD_MAX_LENGTH);
  }

  // fill if present textEdit
  if ((tmp_item = cJSON_GetObjectItem(json, "textEdit"))) {
    item->text_edit = LSP_getTextEditFromJSON(tmp_item);
    item->is_text_edit = true;
  }

  if ((tmp_item = cJSON_GetObjectItem(json, "additionalTextEdits"))) {
    item->additionalTextEditsSize = cJSON_GetArraySize(tmp_item);
    item->additionalTextEdits = malloc(item->additionalTextEditsSize * sizeof(LSP_TextEdit));
    for (int i = 0; i < item->additionalTextEditsSize; i++) {
      item->additionalTextEdits[i] = LSP_getTextEditFromJSON(cJSON_GetArrayItem(tmp_item, i));
    }
  }
}

void LSP_destroyCompletionItem(LSP_CompletionItem* item) {
  if (item->is_text_edit) {
    LSP_destroyTextEdit(item->text_edit);
  }
  for (int j = 0; j < item->additionalTextEditsSize; j++) {
    LSP_destroyTextEdit(item->additionalTextEdits[j]);
  }
  free(item->additionalTextEdits);
  item->additionalTextEdits = NULL;
  item->additionalTextEditsSize = 0;
}


void LSP_destroyCompletionList(LSP_CompletionList* completion_list) {
  for (int i = 0; i < completion_list->completions.size; i++) {
    LSP_destroyCompletionItem(completion_list->completions.items + i);
  }
  free(completion_list->completions.items);

  // defaults
  completion_list->isIncomplete = false;
  completion_list->completions.items = NULL;
  completion_list->completions.size = 0;
}


void LSP_getHoverFromJSON(cJSON* json, LSP_Hover* hover_list) {
  if (!json || cJSON_IsNull(json)) {
    hover_list->size = 0;
    hover_list->contents = NULL;
    hover_list->is_range = false;
    return;
  }

  cJSON* range = cJSON_GetObjectItem(json, "range");
  if (range) {
    hover_list->range = LSP_getRangeFromJSON(range);
    hover_list->is_range = true;
  }
  else {
    hover_list->is_range = false;
  }

  cJSON* contents = cJSON_GetObjectItem(json, "contents");
  if (cJSON_IsArray(contents)) {
    hover_list->size = cJSON_GetArraySize(contents);
    hover_list->contents = malloc(sizeof(LSP_MarkedString) * hover_list->size);
    for (int i = 0; i < hover_list->size; i++) {
      LSP_getMarkedStringFromJSON(cJSON_GetArrayItem(contents, i), &hover_list->contents[i]);
      if (hover_list->contents[i].value[0] == '\0') {
        i--;
        hover_list->size--;
      }
    }
  }
  else {
    hover_list->size = 1;
    hover_list->contents = malloc(sizeof(LSP_MarkedString));
    LSP_getMarkedStringFromJSON(contents, &hover_list->contents[0]);
    if (hover_list->contents[0].value[0] == '\0') {
      hover_list->size = 0;
    }
  }
}

void LSP_getMarkedStringFromJSON(cJSON* json, LSP_MarkedString* item) {
  if (cJSON_IsString(json)) {
    strlcpy(item->value, cJSON_GetStringValue(json), MESSAGE_LENGTH);
    item->documentationType = dt_PLAIN_TEXT;
  }
  else if (cJSON_IsObject(json)) {
    cJSON* kind = cJSON_GetObjectItem(json, "kind");
    cJSON* value = cJSON_GetObjectItem(json, "value");
    if (kind && value) {
      // MarkupContent
      if (strcmp(cJSON_GetStringValue(kind), "markdown") == 0) {
        item->documentationType = dt_MARKDOWN;
      }
      else {
        item->documentationType = dt_PLAIN_TEXT;
      }
      strlcpy(item->value, cJSON_GetStringValue(value), MESSAGE_LENGTH);
    }
    else if (value) {
      // MarkedString with { language, value }

      /* TODO handle MarkedString as a markdown documentationType with
       *  * The pair of a language and a value is an equivalent to markdown:
       *  ```${language}
       *  ${value}
       *  ```
       */
      strlcpy(item->value, cJSON_GetStringValue(value), MESSAGE_LENGTH);
      item->documentationType = dt_PLAIN_TEXT;
    }
  }
}

void LSP_destroyHover(LSP_Hover* hover_list) {
  if (hover_list->contents) {
    free(hover_list->contents);
    hover_list->contents = NULL;
  }
  hover_list->size = 0;
  hover_list->is_range = false;
}


//// -------- Receive Functions --------

bool LSP_dispatchOnReceive(LSP_Server* lsp, void (*dispatcher)(cJSON* packet, LSP_Server* lsp, void* payload),
                           void* payload) {
  if (lsp == NULL) {
    return false;
  }

  cJSON* packet = LSP_readPacketAsJSON(lsp, false);
  if (packet == NULL) {
    return false;
  }
  if (dispatcher != NULL) {
    dispatcher(packet, lsp, payload);
  }
  cJSON_Delete(packet);

  return true;
}


//// -------- Send Functions --------


void LSP_notifyLspFileDidOpen(LSP_Server* lsp, char* file_name, char* file_content) {
  cJSON* request_content = cJSON_CreateObject();

  cJSON* text_document = LSP_getJSONTextDocumentItem(file_name, lsp->language, 1, file_content);
  cJSON_AddItemToObject(request_content, "textDocument", text_document);


  LSP_sendPacketWithJSON(lsp, "textDocument/didOpen", request_content, LSP_NOTIFICATION);

  cJSON_Delete(request_content);
}


void LSP_notifyLspFileDidChange(LSP_Server* lsp, char* file_name, cJSON* array_of_changes, int version) {
  cJSON* request_content = cJSON_CreateObject();

  cJSON* text_document = LSP_getJSONTextDocumentIdentifierVersionned(file_name, version);
  cJSON_AddItemToObject(request_content, "textDocument", text_document);
  cJSON_AddItemToObject(request_content, "contentChanges", array_of_changes);

  LSP_sendPacketWithJSON(lsp, "textDocument/didChange", request_content, LSP_NOTIFICATION);

  cJSON_Delete(request_content);
}


void LSP_requestCompletion(LSP_Server* lsp, char* file_name, LSP_Position pos) {
  cJSON* request_content = cJSON_CreateObject();

  cJSON* text_document = LSP_getJSONTextDocumentIdentifier(file_name);
  cJSON_AddItemToObject(request_content, "textDocument", text_document);

  cJSON* position = LSP_getJSONPosition(pos);
  cJSON_AddItemToObject(request_content, "position", position);

  LSP_PacketID id = LSP_sendPacketWithJSON(lsp, "textDocument/completion", request_content, LSP_REQUEST);

  LSP_addResponseContext(lsp, id, "textDocument/completion", file_name, NULL);

  cJSON_Delete(request_content);
}


void LSP_requestHover(LSP_Server* lsp, char* file_name, LSP_Position pos) {
  cJSON* request_content = cJSON_CreateObject();

  cJSON* text_document = LSP_getJSONTextDocumentIdentifier(file_name);
  cJSON_AddItemToObject(request_content, "textDocument", text_document);

  cJSON* position_json = LSP_getJSONPosition(pos);
  cJSON_AddItemToObject(request_content, "position", position_json);

  LSP_PacketID id = LSP_sendPacketWithJSON(lsp, "textDocument/hover", request_content, LSP_REQUEST);

  LSP_Position* p = malloc(sizeof(LSP_Position));
  *p = pos;

  LSP_addResponseContext(lsp, id, "textDocument/hover", file_name, p);

  cJSON_Delete(request_content);
}


char* LSP_getMethodForGotoType(LSP_GOTO_TYPE goto_type) {
  switch (goto_type) {
    case LSP_GOTO_DECLARATION:
      return "textDocument/declaration";
    case LSP_GOTO_DEFINITION:
      return "textDocument/definition";
    case LSP_GOTO_TYPE_DEFINITION:
      return "textDocument/typeDefinition";
    case LSP_GOTO_IMPLEMENTATION:
      return "textDocument/implementation";
    case LSP_FIND_REFERENCE:
      return "textDocument/references";
  }
  assert(false);
}

void LSP_requestGoto(LSP_Server* lsp, char* file_name, LSP_Position pos, LSP_GOTO_TYPE goto_type) {
  char* goto_method = LSP_getMethodForGotoType(goto_type);

  cJSON* request_content = cJSON_CreateObject();

  cJSON* text_document = LSP_getJSONTextDocumentIdentifier(file_name);
  cJSON_AddItemToObject(request_content, "textDocument", text_document);

  cJSON* position_json = LSP_getJSONPosition(pos);
  cJSON_AddItemToObject(request_content, "position", position_json);

  // little tweakfor find_reference which is not strictly following the same API
  if (goto_type == LSP_FIND_REFERENCE) {
    cJSON* context = cJSON_CreateObject();
    cJSON_AddBoolToObject(context, "includeDeclaration", true);
    cJSON_AddItemToObject(request_content, "context", context);
  }

  LSP_PacketID id = LSP_sendPacketWithJSON(lsp, goto_method, request_content, LSP_REQUEST);

  GotoContext* goto_payload = malloc(sizeof(GotoContext));
  goto_payload->pos = pos;
  goto_payload->goto_type = goto_type;

  LSP_addResponseContext(lsp, id, goto_method, file_name, goto_payload);

  cJSON_Delete(request_content);
}

void LSP_requestFormatting(LSP_Server* lsp, char* file_name, LSP_FormattingOptions options) {
  cJSON* request_content = cJSON_CreateObject();

  cJSON* text_document = LSP_getJSONTextDocumentIdentifier(file_name);
  cJSON_AddItemToObject(request_content, "textDocument", text_document);

  cJSON* formatting_options = cJSON_CreateObject();
  cJSON_AddNumberToObject(formatting_options, "tabSize", options.tabSize);
  cJSON_AddBoolToObject(formatting_options, "insertSpaces", options.insertSpaces);
  cJSON_AddBoolToObject(formatting_options, "trimTrailingWhitespace", options.trimTrailingWhitespace);
  cJSON_AddBoolToObject(formatting_options, "insertFinalNewline", options.insertFinalNewline);
  cJSON_AddBoolToObject(formatting_options, "trimFinalNewlines", options.trimFinalNewlines);

  cJSON_AddItemToObject(request_content, "options", formatting_options);

  LSP_PacketID id = LSP_sendPacketWithJSON(lsp, "textDocument/formatting", request_content, LSP_REQUEST);

  LSP_addResponseContext(lsp, id, "textDocument/formatting", file_name, NULL);

  cJSON_Delete(request_content);
}
