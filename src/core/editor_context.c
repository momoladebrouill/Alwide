#include "editor_context.h"

FileContainer* getActiveFile(EditorContext* ctx) { return ctx->files + ctx->current_file_index; }
