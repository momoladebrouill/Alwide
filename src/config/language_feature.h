#ifndef LANGUAGE_FEATURE_H
#define LANGUAGE_FEATURE_H

#include <stdbool.h>
#include "../../lib/cJSON/cJSON.h"

/**
 * Detection rules for a language.
 */
typedef struct {
  char** extensions;
  int extensions_count;
  char** filenames;
  int filenames_count;
  char** shebangs;
  int shebangs_count;
} ft_Detect;

/**
 * Comment styles for a language.
 */
typedef struct {
  char* line;
  char* block_start;
  char* block_end;
} ft_Comments;

/**
 * Tabulation settings.
 */
typedef struct {
  int size;
  bool use_space;
} ft_Tabulation;

/**
 * Auto-completion pairs (e.g., (), {}, "").
 */
typedef struct {
  char* open;
  char* close;
} ft_Pair;

/**
 * LSP server configuration.
 */
typedef struct {
  char* exe;
  char* arguments;
} ft_LSP;

/**
 * Full language feature definition.
 */
typedef struct {
  char* id;
  char* label;
  ft_Detect detect;
  ft_Comments comments;
  ft_Tabulation tabulation;
  ft_Pair* pairs;
  int pairs_count;
  ft_LSP lsp;
} ft_LanguageFeature;

// Global array of loaded language features
extern ft_LanguageFeature* language_features;
extern int language_features_count;

/**
 * Loads all language features from the JSON configuration file.
 */
void ft_loadLanguageFeatures();

/**
 * Detects the language ID for a given file path and optional first line content (shebang).
 */
const char* ft_detectLanguage(const char* filepath, const char* first_line);

/**
 * Retrieves the language feature structure for a given language ID.
 */
ft_LanguageFeature* ft_getFeatureById(const char* id);

/**
 * Frees all memory allocated for language features.
 */
void ft_freeLanguageFeatures();

// TODO create an API to fetch values from ft_LanguageFeature struct
ft_Tabulation *ft_tab(ft_LanguageFeature* ft);
int ft_tab_size(ft_LanguageFeature* ft);
bool ft_tab_char(ft_LanguageFeature* ft);
// etc ...

#endif // LANGUAGE_FEATURE_H
