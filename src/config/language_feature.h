#ifndef LANGUAGE_FEATURE_H
#define LANGUAGE_FEATURE_H

#include <stdbool.h>
#include "../environnement/constants.h"


#define FT_EXTENSION_MAX_LENGTH 16
#define FT_FILENAME_MAX_LENGTH 100
#define FT_SHEBANG_MAX_LENGTH 100

#define FT_PAIR_MAX_LENGTH 8
#define FT_COMMENTS_MAX_LENGTH 16

/**
 * Detection rules for a language.
 */
typedef struct {
  char (*extensions)[FT_EXTENSION_MAX_LENGTH];
  int extensions_count;
  char (*filenames)[FT_FILENAME_MAX_LENGTH];
  int filenames_count;
  char (*shebangs)[FT_SHEBANG_MAX_LENGTH];
  int shebangs_count;
} ft_Detect;

/**
 * Comment styles for a language.
 */
typedef struct {
  char line[FT_COMMENTS_MAX_LENGTH];
  char block_start[FT_COMMENTS_MAX_LENGTH];
  char block_end[FT_COMMENTS_MAX_LENGTH];
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
  char open[FT_PAIR_MAX_LENGTH];
  char close[FT_PAIR_MAX_LENGTH];
} ft_Pair;

/**
 * LSP server configuration.
 */
typedef struct {
  char exe[LANG_ID_LENGTH];
  char arguments[LANG_ID_LENGTH];
} ft_LSP;

/**
 * Full language feature definition.
 */
typedef struct {
  char id[LANG_ID_LENGTH];
  char label[LANG_ID_LENGTH];
  ft_Detect detect;
  ft_Comments comments;
  ft_Tabulation tabulation;
  ft_Pair *pairs;
  int pairs_count;
  ft_LSP lsp;
} ft_LanguageFeature;

typedef struct {
  ft_LanguageFeature* list;
  int size;
} ft_LanguageFeatureList;

void initLanguageFeatureList(ft_LanguageFeatureList* list);
void destroyLanguageFeatureList(ft_LanguageFeatureList* list);

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
 * Detects and retrieves the language feature structure for a given file path.
 */
ft_LanguageFeature* ft_getFeatureForFile(const char* filepath);

ft_Tabulation* ft_tab(ft_LanguageFeature* ft);
int ft_tab_size(ft_LanguageFeature* ft);
bool ft_tab_use_space(ft_LanguageFeature* ft);

const char* ft_id(ft_LanguageFeature* ft);
const char* ft_label(ft_LanguageFeature* ft);

const char* ft_comment_line(ft_LanguageFeature* ft);
const char* ft_comment_block_start(ft_LanguageFeature* ft);
const char* ft_comment_block_end(ft_LanguageFeature* ft);

int ft_pairs_count(ft_LanguageFeature* ft);
ft_Pair* ft_pair(ft_LanguageFeature* ft, int index);

const char* ft_lsp_exe(ft_LanguageFeature* ft);
const char* ft_lsp_args(ft_LanguageFeature* ft);

#endif // LANGUAGE_FEATURE_H
