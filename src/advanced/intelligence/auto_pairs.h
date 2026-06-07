#ifndef AUTO_PAIRS_H
#define AUTO_PAIRS_H

#include "../../data-management/file_management.h"
#include "../../data-management/file_structure.h"

/**
 * Handles auto-pairing of characters (e.g., inserting '}' after '{').
 * Returns true if the input was handled (either via auto-pairing or overtyping).
 */
bool ilj_handleAutoPairs(FileContainer* fc, Char_U8 input, History** history_p,
                         PayloadStateChange payload_state_change);

/**
 * Handles pair-aware backspace deletion.
 * When the cursor is positioned between a matched open/close pair (e.g. '(|)'),
 * deletes both the opening character to the left and the closing character to the right.
 * Returns true if both characters were consumed, false if normal backspace should proceed.
 */
bool ilj_handleAutoPairDelete(FileContainer* fc, History** history_p, PayloadStateChange payload_state_change);

#endif // AUTO_PAIRS_H
