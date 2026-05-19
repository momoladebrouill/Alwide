#include "language_feature.h"
#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../environnement/global_variables.h"
#include "../utils/tools.h"
#include "config.h"

ft_LanguageFeature* language_features = NULL;
int language_features_count = 0;

static ft_LanguageFeature default_feature = {
  .id = "plain",
  .label = "Plain Text",
  .detect = {NULL, 0, NULL, 0, NULL, 0},
  .comments = {.line = "//", .block_start = "/*", .block_end = "*/"}, // Reasonable defaults
  .tabulation = {.size = 2, .use_space = true},
  .pairs = NULL,
  .pairs_count = 0,
  .lsp = {.exe = "", .arguments = ""}};

/**
 * Helper to parse a JSON array of strings into a C array of strings.
 */
static char** parseStringArray(cJSON* array, int* count) {
  if (!cJSON_IsArray(array)) {
    *count = 0;
    return NULL;
  }
  *count = cJSON_GetArraySize(array);
  if (*count == 0) {
    return NULL;
  }

  char** result = malloc(sizeof(char*) * (*count));
  for (int i = 0; i < *count; i++) {
    cJSON* item = cJSON_GetArrayItem(array, i);
    result[i] = strdup(cJSON_GetStringValue(item));
  }
  return result;
}

/**
 * Helper to free a C array of strings.
 */
static void freeStringArray(char** array, int count) {
  if (!array) {
    return;
  }
  for (int i = 0; i < count; i++) {
    free(array[i]);
  }
  free(array);
}

void ft_loadLanguageFeatures() {
  const char* home = getenv("HOME");
  if (!home) {
    return;
  }

  // Fetching the folder where to load theme and queries.
  char* load_path = cJSON_GetStringValue(cJSON_GetObjectItem(config, "default_path"));

  char path[PATH_MAX];
  // Loading features
  snprintf(path, sizeof(path), "%s/languages-features.json", load_path);

  long length;
  char* content = loadFullFile(path, &length);
  if (!content) {
    fprintf(stderr, "Failed to load languages-features.json from %s.\n", path);
    return;
  }

  cJSON* json = cJSON_Parse(content);
  free(content);
  if (!json) {
    return;
  }

  language_features_count = cJSON_GetArraySize(json);
  language_features = malloc(sizeof(ft_LanguageFeature) * language_features_count);

  for (int i = 0; i < language_features_count; i++) {
    // TODO change the ft_LanguageFeature to use static maximum buffer size instead of strdup that store all on the heap
    cJSON* lang_json = cJSON_GetArrayItem(json, i);
    ft_LanguageFeature* lang = &language_features[i];

    lang->id = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(lang_json, "id")));
    lang->label = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(lang_json, "label")));

    cJSON* detect = cJSON_GetObjectItem(lang_json, "detect");
    lang->detect.extensions =
      parseStringArray(cJSON_GetObjectItem(detect, "extensions"), &lang->detect.extensions_count);
    lang->detect.filenames = parseStringArray(cJSON_GetObjectItem(detect, "file-name"), &lang->detect.filenames_count);
    lang->detect.shebangs = parseStringArray(cJSON_GetObjectItem(detect, "shebang"), &lang->detect.shebangs_count);

    cJSON* comments = cJSON_GetObjectItem(lang_json, "comments");
    lang->comments.line = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(comments, "line")));

    cJSON* block = cJSON_GetObjectItem(comments, "block");
    if (cJSON_IsArray(block) && cJSON_GetArraySize(block) == 2) {
      lang->comments.block_start = strdup(cJSON_GetStringValue(cJSON_GetArrayItem(block, 0)));
      lang->comments.block_end = strdup(cJSON_GetStringValue(cJSON_GetArrayItem(block, 1)));
    }
    else {
      lang->comments.block_start = strdup("");
      lang->comments.block_end = strdup("");
    }

    cJSON* tab = cJSON_GetObjectItem(lang_json, "tabulation");
    lang->tabulation.size = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(tab, "size"));
    lang->tabulation.use_space = cJSON_IsTrue(cJSON_GetObjectItem(tab, "space"));

    cJSON* pairs = cJSON_GetObjectItem(lang_json, "pairs");
    lang->pairs_count = cJSON_GetArraySize(pairs);
    lang->pairs = malloc(sizeof(ft_Pair) * lang->pairs_count);
    for (int j = 0; j < lang->pairs_count; j++) {
      cJSON* pair = cJSON_GetArrayItem(pairs, j);
      lang->pairs[j].open = strdup(cJSON_GetStringValue(cJSON_GetArrayItem(pair, 0)));
      lang->pairs[j].close = strdup(cJSON_GetStringValue(cJSON_GetArrayItem(pair, 1)));
    }

    cJSON* lsp = cJSON_GetObjectItem(lang_json, "lsp");
    lang->lsp.exe = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(lsp, "exe")));

    cJSON* args_arr = cJSON_GetObjectItem(lsp, "arguments");
    int args_count = cJSON_GetArraySize(args_arr);
    if (args_count > 0) {
      char args_buf[1024] = {0};
      for (int j = 0; j < args_count; j++) {
        strcat(args_buf, cJSON_GetStringValue(cJSON_GetArrayItem(args_arr, j)));
        if (j < args_count - 1) {
          strcat(args_buf, " ");
        }
      }
      lang->lsp.arguments = strdup(args_buf);
    }
    else {
      lang->lsp.arguments = strdup("");
    }
  }

  cJSON_Delete(json);
}

ft_LanguageFeature* ft_getFeatureById(const char* id) {
  if (id) {
    for (int i = 0; i < language_features_count; i++) {
      if (strcmp(language_features[i].id, id) == 0) {
        return &language_features[i];
      }
    }
  }
  return &default_feature;
}

const char* ft_detectLanguage(const char* filepath, const char* first_line) {
  if (!filepath) {
    return NULL;
  }

  char* bname = strdup(filepath);
  char* filename = basename(bname);
  char* dot = strrchr(filename, '.');

  const char* found_id = NULL;

  for (int i = 0; i < language_features_count; i++) {
    ft_LanguageFeature* lang = &language_features[i];

    // 1. Match filenames
    for (int j = 0; j < lang->detect.filenames_count; j++) {
      if (strcmp(filename, lang->detect.filenames[j]) == 0) {
        found_id = lang->id;
        goto cleanup;
      }
    }

    // 2. Match extensions
    if (dot) {
      for (int j = 0; j < lang->detect.extensions_count; j++) {
        if (strcmp(dot, lang->detect.extensions[j]) == 0) {
          found_id = lang->id;
          goto cleanup;
        }
      }
    }

    // 3. Match shebangs
    if (first_line && first_line[0] == '#' && first_line[1] == '!') {
      for (int j = 0; j < lang->detect.shebangs_count; j++) {
        if (strstr(first_line, lang->detect.shebangs[j]) != NULL) {
          found_id = lang->id;
          goto cleanup;
        }
      }
    }
  }

cleanup:
  free(bname);
  return found_id;
}

void ft_freeLanguageFeatures() {
  for (int i = 0; i < language_features_count; i++) {
    ft_LanguageFeature* lang = &language_features[i];
    free(lang->id);
    free(lang->label);
    freeStringArray(lang->detect.extensions, lang->detect.extensions_count);
    freeStringArray(lang->detect.filenames, lang->detect.filenames_count);
    freeStringArray(lang->detect.shebangs, lang->detect.shebangs_count);
    free(lang->comments.line);
    free(lang->comments.block_start);
    free(lang->comments.block_end);
    for (int j = 0; j < lang->pairs_count; j++) {
      free(lang->pairs[j].open);
      free(lang->pairs[j].close);
    }
    free(lang->pairs);
    free(lang->lsp.exe);
    free(lang->lsp.arguments);
  }
  free(language_features);
  language_features = NULL;
  language_features_count = 0;
}
