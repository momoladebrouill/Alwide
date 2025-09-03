#ifndef WISHWIM_TREE_SITTER_HIGHLIGHTER_H
#define WISHWIM_TREE_SITTER_HIGHLIGHTER_H

#include <ncurses.h>

#include "../../terminal/highlight.h"
#include "tree_manager.h"

void TS_highlightCurrentFile(TS_Data* highlight_data, WINDOW* ftw, int screen_x, int screen_y, Cursor cursor,
                             WindowHighlightDescriptor* highlight_descriptor);

#endif // WISHWIM_TREE_SITTER_HIGHLIGHTER_H
