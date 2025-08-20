#include "tree_sitter_highlighter.h"

#include <assert.h>
#include <string.h>
#include <time.h>

#include "tree_query.h"
#include "../../utils/constants.h"

void saveCaptureToHighlightDescriptor(HighlightThemeList theme_list, Cursor tmp, TSNode node, const char* result,
                                      WindowHighlightDescriptor* highlight_descriptor, uint16_t priority) {
  if (result == NULL) {
    return;
  }

  attr_t attr = A_NORMAL;
  NCURSES_PAIRS_T color = DEFAULT_COLOR_PAIR;

  bool found = false;
  // Setup style for group found.
  for (int i = 0; i < theme_list.size; i++) {
    if (strcmp(theme_list.groups[i].group, result) == 0) {
      attr |= getAttrForTheme(theme_list.groups[i]);
      color = theme_list.groups[i].color_n;

      found = true;
      break;
    }
  }

  if (found == false) {
    // Quit if a query was found, but not theme for this group was found.
    fprintf(stderr, "      || No theme found for capture %s  || \n", result);
    return;
  }

  TSPoint start_point = ts_node_start_point(node);
  TSPoint end_point = ts_node_end_point(node);

  Cursor begin_cursor = moveRight(byteCursorToCursor(tmp, start_point.row, start_point.column));
  Cursor end_cursor = byteCursorToCursor(tmp, end_point.row, end_point.column);
  whd_insertDescriptor(highlight_descriptor, begin_cursor, end_cursor, color, attr, priority, true);
}

void executeHighlightQuery(TSQuery* query, TSQueryCursor* qcursor, RegexMap* regex_map, HighlightThemeList theme_list,
                           Cursor cursor, WindowHighlightDescriptor* highlight_descriptor) {
  Cursor tmp = cursor;

  TSQueryMatch query_match;
  while (TSQueryCursorNextMatchWithPredicates(&tmp, query, qcursor, &query_match, regex_map)) {
    for (int index = 0; index < query_match.capture_count; index++) {
      TSNode node = query_match.captures[index].node;
      // fprintf(stderr, "Node (%d, %d) -> (%d, %d) : (index: %d) \n", ts_node_start_point(node).row, ts_node_start_point(node).column, ts_node_end_point(node).row,

      uint32_t size = 0;
      const char* result = ts_query_capture_name_for_id(query, query_match.captures[index].index, &size);
      // fprintf(stderr, " -> capture name : %s\n", result);

      uint16_t priority = query_match.captures[index].index;
      // If a capture.
      saveCaptureToHighlightDescriptor(theme_list, tmp, node, result, highlight_descriptor, priority);
    }
  }
}


void highlightCurrentFile(FileHighlightDatas* highlight_data, WINDOW* ftw, int screen_x, int screen_y, Cursor cursor,
                          WindowHighlightDescriptor* highlight_descriptor) {
  if (highlight_data->is_active == false) {
    return;
  }

  clock_t t;
  t = clock();

  // setup parser to iterate on captures
  ParserContainer* parser = getParserForLanguage(&parsers, highlight_data->lang_id);
  assert(parser != NULL);

  int width = getmaxx(ftw);
  int height = getmaxy(ftw);

  // Setup cursor range
  TSPoint begin;
  begin.row = screen_y - 1;
  begin.column = screen_x;
  TSPoint end;
  end.row = screen_y + height - 2;
  end.column = screen_x + width;
  ts_query_cursor_set_point_range(
    parser->cursor,
    begin,
    end
  );

  ts_query_cursor_exec(parser->cursor, parser->queries, ts_tree_root_node(highlight_data->tree));

  // execute the query search.
  executeHighlightQuery(parser->queries, parser->cursor, &parser->regex_map, parser->theme_list, cursor, highlight_descriptor);

  t = clock() - t;
  double time_taken = ((double)t) / CLOCKS_PER_SEC * 1000; // in millis

//  fprintf(stderr, "highlight() took %f millis to execute part of check for highlight. \n", time_taken);
}
