#include "search.h"
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/**
 * Case-insensitive substring search (strstr equivalent for case-insensitive matching).
 * Matches character-by-character by converting to lowercase.
 * Named local_strcasestr to avoid symbol conflict with the GNU extension in <string.h>.
 *
 * @param haystack The string to search within.
 * @param needle The string to search for.
 * @return A pointer to the first occurrence of the needle, or NULL if not found.
 */
static char* local_strcasestr(const char* haystack, const char* needle) {
  if (!*needle) {
    return (char*)haystack;
  }
  for (; *haystack; haystack++) {
    if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
      const char *h, *n;
      for (h = haystack, n = needle; *h && *n; h++, n++) {
        if (tolower((unsigned char)*h) != tolower((unsigned char)*n)) {
          break;
        }
      }
      if (!*n) {
        return (char*)haystack;
      }
    }
  }
  return NULL;
}

/**
 * Unified substring locator supporting both case-sensitive and case-insensitive search.
 */
static const char* find_substring(const char* haystack, const char* needle, bool case_sensitive) {
  if (case_sensitive) {
    return strstr(haystack, needle);
  }
  else {
    return local_strcasestr(haystack, needle);
  }
}

/**
 * Converts a byte offset within a UTF-8 string to a character offset.
 * Alwide represents file contents in UTF-8, so multi-byte characters occupy
 * multiple bytes but represent a single cursor/character column.
 */
static int byteOffsetToCharOffset(const char* str, int byte_offset) {
  int char_offset = 0;
  for (int i = 0; i < byte_offset; i++) {
    // UTF-8 continuation bytes start with 10xxxxxx (0x80 to 0xBF).
    // We only increment the character count for non-continuation bytes.
    if ((str[i] & 0xC0) != 0x80) {
      char_offset++;
    }
  }
  return char_offset;
}

/**
 * Counts the total number of characters in a null-terminated UTF-8 string.
 */
static int byteCountToCharCount(const char* str) {
  int char_count = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    if ((str[i] & 0xC0) != 0x80) {
      char_count++;
    }
  }
  return char_count;
}

/**
 * Traverses a linked list of LineNode structures starting at `line` and flattens
 * the entire line's UTF-8 character content into a single dynamically allocated buffer.
 *
 * @param line The start of the LineNode list representing the line.
 * @return A dynamically allocated string containing the full line content, or NULL on failure.
 *         The caller is responsible for calling free() on the returned buffer.
 */
static char* getLineString(LineNode* line) {
  int total_bytes = 0;
  LineNode* curr = line;
  while (curr != NULL) {
    total_bytes += curr->byte_count;
    curr = curr->next;
  }

  char* buf = malloc(total_bytes + 1);
  if (!buf) {
    return NULL;
  }

  int offset = 0;
  curr = line;
  while (curr != NULL) {
    for (int i = 0; i < curr->element_number; i++) {
      Char_U8 ch = curr->ch[i];
      int sz = sizeChar_U8(ch);
      for (int b = 0; b < sz; b++) {
        buf[offset++] = ch.t[b];
      }
    }
    curr = curr->next;
  }
  buf[offset] = '\0';
  return buf;
}

/**
 * Searches the file forward starting from the current cursor position to find the next match.
 * If a match is found, updates start_cursor and end_cursor to span the match and returns true.
 *
 * @param fc The file container containing the buffer and cursor state.
 * @param query The text query to search for.
 * @param case_sensitive Whether to perform a case-sensitive search.
 * @param wrap Whether to wrap to the start of the file when reaching the end.
 * @param start_cursor Output pointer to receive the start of the matched term.
 * @param end_cursor Output pointer to receive the end of the matched term.
 * @return true if a match was found and cursors updated, false otherwise.
 */
bool ilj_findNext(FileContainer* fc, const char* query, bool case_sensitive, bool wrap, Cursor* start_cursor,
                  Cursor* end_cursor) {
  if (!query || *query == '\0') {
    return false;
  }

  // Determine the total number of lines in the file by seeking to the absolute end.
  Cursor end_finder = tryToReachAbsPosition(fc->cursor, INT_MAX, INT_MAX);
  int total_lines = cursor_row(end_finder);
  if (total_lines <= 0) {
    return false;
  }

  int start_row = cursor_row(fc->cursor);
  int start_col = cursor_col(fc->cursor);

  int current_row = start_row;
  bool first_line = true;
  int query_char_len = byteCountToCharCount(query);

  while (true) {
    // Retrieve the text representation of the current row
    Cursor temp_cursor = tryToReachAbsPosition(fc->cursor, current_row, 0);
    char* line_str = getLineString(temp_cursor.line_id.line);
    if (!line_str) {
      return false;
    }

    const char* scan_ptr = line_str;

    // Scan for all occurrences of the query on the current line
    while (true) {
      const char* match = find_substring(scan_ptr, query, case_sensitive);
      if (!match) {
        break;
      }

      int byte_offset = match - line_str;
      int char_offset = byteOffsetToCharOffset(line_str, byte_offset);

      // On the initial line, skip any matches that start before the starting column.
      if (first_line && char_offset < start_col) {
        scan_ptr = match + 1;
        continue;
      }

      // Found a valid match! Populate outputs and clean up.
      *start_cursor = tryToReachAbsPosition(fc->cursor, current_row, char_offset);
      *end_cursor = tryToReachAbsPosition(fc->cursor, current_row, char_offset + query_char_len);
      free(line_str);
      return true;
    }

    free(line_str);

    // Move to the next row forward
    current_row++;
    first_line = false;

    // Handle wrapping around to the first line if enabled
    if (current_row > total_lines) {
      if (wrap) {
        current_row = 1;
      }
      else {
        break;
      }
    }

    // If we've circled back to the starting line, we must perform a final check.
    // We search the starting line, but only return the first match that lies
    // strictly before our original starting column (`char_offset < start_col`).
    if (current_row == start_row && !first_line) {
      temp_cursor = tryToReachAbsPosition(fc->cursor, start_row, 0);
      line_str = getLineString(temp_cursor.line_id.line);
      if (line_str) {
        scan_ptr = line_str;
        while (true) {
          const char* match = find_substring(scan_ptr, query, case_sensitive);
          if (!match) {
            break;
          }

          int byte_offset = match - line_str;
          int char_offset = byteOffsetToCharOffset(line_str, byte_offset);

          // Stop if the match goes past or equal to our starting column
          if (char_offset >= start_col) {
            break;
          }

          *start_cursor = tryToReachAbsPosition(fc->cursor, start_row, char_offset);
          *end_cursor = tryToReachAbsPosition(fc->cursor, start_row, char_offset + query_char_len);
          free(line_str);
          return true;
        }
        free(line_str);
      }
      break;
    }
  }

  return false;
}

/**
 * Searches the file backward starting from the current cursor position to find the previous match.
 * If a match is found, updates start_cursor and end_cursor to span the match and returns true.
 *
 * @param fc The file container containing the buffer and cursor state.
 * @param query The text query to search for.
 * @param case_sensitive Whether to perform a case-sensitive search.
 * @param wrap Whether to wrap to the end of the file when reaching the beginning.
 * @param start_cursor Output pointer to receive the start of the matched term.
 * @param end_cursor Output pointer to receive the end of the matched term.
 * @return true if a match was found and cursors updated, false otherwise.
 */
bool ilj_findPrev(FileContainer* fc, const char* query, bool case_sensitive, bool wrap, Cursor* start_cursor,
                  Cursor* end_cursor) {
  if (!query || *query == '\0') {
    return false;
  }

  // Determine the total number of lines in the file by seeking to the absolute end.
  Cursor end_finder = tryToReachAbsPosition(fc->cursor, INT_MAX, INT_MAX);
  int total_lines = cursor_row(end_finder);
  if (total_lines <= 0) {
    return false;
  }

  // Start checking from the current cursor's row and column
  int start_row = cursor_row(fc->cursor);
  int start_col = cursor_col(fc->cursor);

  // If a selection is active, we must begin searching before the selection start to avoid
  // matching the currently selected term repeatedly.
  if (!cursor_is_disabled(fc->select_cursor)) {
    Cursor min_cur = cursor_min(fc->cursor, fc->select_cursor);
    start_row = cursor_row(min_cur);
    start_col = cursor_col(min_cur);
  }

  int current_row = start_row;
  bool first_line = true;
  int query_char_len = byteCountToCharCount(query);

  while (true) {
    // Retrieve the text representation of the current row
    Cursor temp_cursor = tryToReachAbsPosition(fc->cursor, current_row, 0);
    char* line_str = getLineString(temp_cursor.line_id.line);
    if (!line_str) {
      return false;
    }

    const char* best_match = NULL;
    const char* scan_ptr = line_str;

    // Scan the entire line to find the LAST match that is strictly before the starting column.
    while (true) {
      const char* match = find_substring(scan_ptr, query, case_sensitive);
      if (!match) {
        break;
      }

      int byte_offset = match - line_str;
      int char_offset = byteOffsetToCharOffset(line_str, byte_offset);

      // On the initial line, any matches starting at or after `start_col` are ignored
      if (first_line && char_offset >= start_col) {
        break;
      }

      best_match = match;
      scan_ptr = match + 1; // Scan next characters to see if there's a later match on the same line
    }

    // If we found a valid match on this line, select it!
    if (best_match) {
      int byte_offset = best_match - line_str;
      int char_offset = byteOffsetToCharOffset(line_str, byte_offset);
      *start_cursor = tryToReachAbsPosition(fc->cursor, current_row, char_offset);
      *end_cursor = tryToReachAbsPosition(fc->cursor, current_row, char_offset + query_char_len);
      free(line_str);
      return true;
    }

    free(line_str);

    // Move to the previous row backwards
    current_row--;
    first_line = false;

    // Handle wrapping around to the end of the file if enabled
    if (current_row < 1) {
      if (wrap) {
        current_row = total_lines;
      }
      else {
        break;
      }
    }

    // If we've circled back to the starting line, we must perform a final check.
    // We search the starting line, but only return the last match that lies
    // strictly at or after our original starting column (`char_offset >= start_col`).
    if (current_row == start_row && !first_line) {
      temp_cursor = tryToReachAbsPosition(fc->cursor, start_row, 0);
      line_str = getLineString(temp_cursor.line_id.line);
      if (line_str) {
        scan_ptr = line_str;
        best_match = NULL;
        while (true) {
          const char* match = find_substring(scan_ptr, query, case_sensitive);
          if (!match) {
            break;
          }

          int byte_offset = match - line_str;
          int char_offset = byteOffsetToCharOffset(line_str, byte_offset);

          // We only look at matches that start at or after our starting column.
          if (char_offset >= start_col) {
            best_match = match;
          }
          scan_ptr = match + 1;
        }

        if (best_match) {
          int byte_offset = best_match - line_str;
          int char_offset = byteOffsetToCharOffset(line_str, byte_offset);
          *start_cursor = tryToReachAbsPosition(fc->cursor, start_row, char_offset);
          *end_cursor = tryToReachAbsPosition(fc->cursor, start_row, char_offset + query_char_len);
          free(line_str);
          return true;
        }
        free(line_str);
      }
      break;
    }
  }

  return false;
}
