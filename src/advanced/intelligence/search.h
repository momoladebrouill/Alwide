#ifndef ALWIDE_INTEL_SEARCH_H
#define ALWIDE_INTEL_SEARCH_H

#include "../../data-management/file_management.h"

/**
 * Searches the file forward starting from the current cursor position to find the next match.
 * If found, returns true and populates start_cursor and end_cursor.
 * If wrap is true, it wraps around to the beginning of the file.
 */
bool ilj_findNext(FileContainer* fc, const char* query, bool case_sensitive, bool wrap, Cursor* start_cursor,
                  Cursor* end_cursor);

/**
 * Searches the file backward starting from the current cursor position to find the previous match.
 * If found, returns true and populates start_cursor and end_cursor.
 * If wrap is true, it wraps around to the end of the file.
 */
bool ilj_findPrev(FileContainer* fc, const char* query, bool case_sensitive, bool wrap, Cursor* start_cursor,
                  Cursor* end_cursor);

#endif // ALWIDE_INTEL_SEARCH_H
