#ifndef WISHWIM_GLOBALS_H
#define WISHWIM_GLOBALS_H

#include "../advanced/lsp/lsp_handler.h"
#include "../advanced/tree-sitter/tree_manager.h"
#include "../io-management/workspace_settings.h"
#include "../config/language_feature.h"


extern cJSON* config;
extern ParserList parsers;
extern LSPServerLinkedList lsp_servers;
extern WorkspaceSettings workspace_settings;
extern ft_LanguageFeatureList language_features;


#endif // WISHWIM_GLOBALS_H
