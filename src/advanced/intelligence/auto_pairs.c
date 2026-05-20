#include "auto_pairs.h"
#include <string.h>
#include "../../terminal/term_handler.h"
#include "../../data-management/utf_8_extractor.h"

bool ft_handleAutoPairs(FileContainer* fc, Char_U8 input, History** history_p, PayloadStateChange payload_state_change) {
  if (!fc->feature || fc->feature->pairs_count == 0) {
    return false;
  }

  // 1. Check for overtyping a closing character
  Char_U8 current_ch = {0};
  if (fc->cursor.line_id.relative_column < fc->cursor.line_id.line->element_number) {
    current_ch = fc->cursor.line_id.line->ch[fc->cursor.line_id.relative_column];
  } else if (fc->cursor.line_id.line->next != NULL && fc->cursor.line_id.line->next->element_number > 0) {
    current_ch = fc->cursor.line_id.line->next->ch[0];
  }

  for (int i = 0; i < fc->feature->pairs_count; i++) {
    ft_Pair* pair = &fc->feature->pairs[i];
    Char_U8 close_u8 = readChar_U8FromCharArray(pair->close);
    if (areChar_U8Equals(input, close_u8) && areChar_U8Equals(input, current_ch)) {
      fc->cursor = moveRight(fc->cursor);
      return true;
    }
  }

  // 2. Check for inserting an opening character
  for (int i = 0; i < fc->feature->pairs_count; i++) {
    ft_Pair* pair = &fc->feature->pairs[i];
    Char_U8 open_u8 = readChar_U8FromCharArray(pair->open);
    
    if (areChar_U8Equals(input, open_u8)) {
      char combined[9] = {0};
      strcat(combined, pair->open);
      strcat(combined, pair->close);

      fc->cursor = insertCharArrayAtCursorWithHist(history_p, fc->cursor, combined, payload_state_change, ft_tab(fc->feature));
      
      // Move cursor back one position to be between the pairs
      fc->cursor = moveLeft(fc->cursor);
      return true;
    }
  }

  return false;
}
