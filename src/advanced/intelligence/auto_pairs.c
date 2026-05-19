#include "auto_pairs.h"
#include <string.h>
#include "../../terminal/term_handler.h"

bool ft_handleAutoPairs(FileContainer* fc, Char_U8 input, History** history_p, PayloadStateChange payload_state_change) {
  if (!fc->feature || fc->feature->pairs_count == 0) {
    return false;
  }

  // TODO you may use areCharU8Equal and use a CharU8 function to convert a char to a charu8
  // Convert Char_U8 to string for comparison
  char input_str[5] = {0};
  memcpy(input_str, input.t, 4);

  // 1. Check for overtyping a closing character
  Char_U8 current_ch = {0};
  if (fc->cursor.line_id.relative_column < fc->cursor.line_id.line->element_number) {
    current_ch = fc->cursor.line_id.line->ch[fc->cursor.line_id.relative_column];
  } else if (fc->cursor.line_id.line->next != NULL && fc->cursor.line_id.line->next->element_number > 0) {
    current_ch = fc->cursor.line_id.line->next->ch[0];
  }
  
  char current_str[5] = {0};
  memcpy(current_str, current_ch.t, 4);

  for (int i = 0; i < fc->feature->pairs_count; i++) {
    ft_Pair* pair = &fc->feature->pairs[i];
    if (strcmp(input_str, pair->close) == 0 && strcmp(input_str, current_str) == 0) {
      fc->cursor = moveRight(fc->cursor);
      return true;
    }
  }

  // 2. Check for inserting an opening character
  for (int i = 0; i < fc->feature->pairs_count; i++) {
    ft_Pair* pair = &fc->feature->pairs[i];
    
    if (strcmp(input_str, pair->open) == 0) {
      char combined[9] = {0};
      strcat(combined, input_str);
      strcat(combined, pair->close);

      fc->cursor = insertCharArrayAtCursorWithHist(history_p, fc->cursor, combined, payload_state_change, fc->feature->tabulation.size, fc->feature->tabulation.use_space);
      
      // Move cursor back one position to be between the pairs
      fc->cursor = moveLeft(fc->cursor);
      return true;
    }
  }

  return false;
}
